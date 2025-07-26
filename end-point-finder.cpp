#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <curl/curl.h>

#define TIMEOUT_SEC 8
#define MAX_THREADS 20
#define MAX_REDIRS 5

struct ScanData {
    std::string url;
    int http_status;
    std::string content_type;
    double load_time;
    size_t data_size;
};

std::vector<ScanData> found_endpoints;
std::mutex data_mutex;

std::vector<std::string> common_paths = {
    "/", "/admin", "/login", "/api", "/wp-admin", "/test",
    "/backup", "/config", "/env", "/internal", "/secret",
    "/debug", "/console", "/manager", "/phpmyadmin",
    "/.git", "/.env", "/.htaccess", "/robots.txt",
    "/v1", "/v2", "/v3", "/beta", "/staging",
    "/uploads", "/images", "/assets", "/static",
    "/cgi-bin", "/bin", "/cmd", "/shell",
    "/owa", "/ecp", "/ews", "/exchange"
};

size_t curl_data_discard(void* ptr, size_t size, size_t nmemb, void* userdata) {
    return size * nmemb;
}

void check_url(const std::string& base_url, const std::string& path) {
    CURL* curl_handle = curl_easy_init();
    if (!curl_handle) return;

    std::string full_url = base_url + path;
    long response_code = 0;
    char* ct_header = nullptr;
    double timing = 0;
    size_t bytes_received = 0;

    curl_easy_setopt(curl_handle, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_data_discard);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, TIMEOUT_SEC);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, MAX_REDIRS);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);

    if (curl_easy_perform(curl_handle) == CURLE_OK) {
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct_header);
        curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &timing);
        curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD, &bytes_received);

        if (response_code != 404) {
            std::lock_guard<std::mutex> lock(data_mutex);
            found_endpoints.push_back({
                full_url,
                static_cast<int>(response_code),
                ct_header ? ct_header : "n/a",
                timing,
                bytes_received
            });
        }
    }

    curl_easy_cleanup(curl_handle);
}

std::vector<std::string> get_custom_paths(const std::string& filename) {
    std::vector<std::string> custom_paths;
    std::ifstream input_file(filename);
    std::string line;

    if (input_file) {
        while (std::getline(input_file, line)) {
            if (!line.empty() && line[0] != '#') {
                custom_paths.push_back(line);
            }
        }
    }
    
    return custom_paths;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <base_url> [wordlist_file]\n";
        return 1;
    }

    std::string target_url = argv[1];
    std::vector<std::string> scan_paths = common_paths;

    if (argc > 2) {
        auto extra_paths = get_custom_paths(argv[2]);
        scan_paths.insert(scan_paths.end(), extra_paths.begin(), extra_paths.end());
    }

    std::sort(scan_paths.begin(), scan_paths.end());
    scan_paths.erase(std::unique(scan_paths.begin(), scan_paths.end()), scan_paths.end());

    std::cout << "Scanning " << target_url << " (" << scan_paths.size() << " paths)\n";

    auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> workers;

    for (const auto& path : scan_paths) {
        workers.emplace_back(check_url, target_url, path);
        
        if (workers.size() >= MAX_THREADS) {
            for (auto& t : workers) t.join();
            workers.clear();
        }
    }

    for (auto& t : workers) t.join();
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "\nScan completed in " << elapsed.count() << " seconds\n";
    std::cout << "Found " << found_endpoints.size() << " accessible endpoints\n\n";

    std::map<int, std::vector<ScanData>> grouped_results;
    for (const auto& result : found_endpoints) {
        grouped_results[result.http_status].push_back(result);
    }

    for (const auto& group : grouped_results) {
        std::cout << "=== Status " << group.first << " ===\n";
        for (const auto& item : group.second) {
            std::cout << item.url << " (" << item.load_time << "s)\n";
            std::cout << "Type: " << item.content_type << " | Size: " << item.data_size << " bytes\n\n";
        }
    }

    return 0;
}
