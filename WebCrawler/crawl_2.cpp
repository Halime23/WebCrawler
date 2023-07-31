#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <algorithm>
#include <condition_variable>
#include <curl/curl.h>
#include <atomic>
#include <memory>
#include <limits>

const std::string URL_PREFIX = "https://en.wikipedia.org/wiki/Elon_Musk";
const std::string SEARCH_WORD = "co-founder";
const int MAX_DEPTH = 2;
const int MAX_THREADS = 10;
const int MAX_OUTPUT = MAX_THREADS; // Maksimum çıktı sayısı

std::queue<std::string> urls;
std::vector<std::string> visited;
std::mutex urlsMutex;
std::mutex visitedMutex;
std::condition_variable cv;
int outputCount = 0;                   // Çıktı sayısını tutmak için sayaç
std::atomic<bool> stopCrawling(false); // Taramayı durdurmak için flag

size_t writeCallback(char *buf, size_t size, size_t nmemb, void *up)
{
    std::string *str = static_cast<std::string *>(up);
    size_t len = size * nmemb;

    // Bellek sınırlarını kontrol edelim
    if (len > (std::numeric_limits<size_t>::max() - str->size()))
    {
        // Bellek taşması durumu, işlemi durduralım veya gerekli hata işlemlerini yapalım.
    }
    else
    {
        str->append(buf, len);
    }

    return len;
}

void crawl()
{
    while (true)
    {
        // A URL is retrieved from the URL queue
        std::unique_lock<std::mutex> lock(urlsMutex);
        cv.wait(lock, []
                { return !urls.empty() || stopCrawling; });
        if (urls.empty())
        {
            lock.unlock();
            break; // Linklerin tamamı tarandığında döngüden çık
        }
        std::string url = urls.front();
        urls.pop();
        lock.unlock();

        // Added to the list of visited URLs
        std::lock_guard<std::mutex> visitedLock(visitedMutex);
        visited.push_back(url);

        // Depth control is done
        int depth = std::count(url.begin(), url.end(), '/') - 2;
        if (depth > MAX_DEPTH)
        {
            continue;
        }

        // URL is visited
        CURL *curl;
        curl = curl_easy_init();
        if (curl)
        {
            std::unique_ptr<std::string> html = std::make_unique<std::string>();
            CURLcode res;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, html.get());
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            // Word search in HTML
            size_t pos = 0;
            int wordCount = 0;
            while ((pos = html->find(SEARCH_WORD, pos)) != std::string::npos)
            {
                pos += SEARCH_WORD.length();
                wordCount++;
            }

            // Results are printed
            if (wordCount > 0)
            {
                std::lock_guard<std::mutex> outputLock(urlsMutex);
                if (outputCount < MAX_OUTPUT)
                {
                    std::cout << "URL: " << url << ", Number of Word: " << wordCount << std::endl;
                    outputCount++;
                }
                if (outputCount >= MAX_OUTPUT)
                {
                    stopCrawling = true; // Maksimum çıktı sayısına ulaşıldığında taramayı durdurmak için flag'i set ediyoruz
                    cv.notify_all();     // Diğer thread'leri uyandırarak programın sonlanmasını sağlıyoruz
                    break;
                }
            }

            // The links are parsed and added to the URL queue
            size_t start = html->find("<a href=\"");
            while (start != std::string::npos)
            {
                size_t end = html->find("\"", start + 9);
                if (end != std::string::npos)
                {
                    std::string link = html->substr(start + 9, end - start - 9);

                    if (link.find("http") != 0)
                    {
                        link = URL_PREFIX + link;
                    }

                    std::lock_guard<std::mutex> urlsLock(urlsMutex);
                    if (std::find(visited.begin(), visited.end(), link) == visited.end())
                    {
                        urls.push(link);
                        cv.notify_one();
                    }
                }
                start = html->find("<a href=\"", end);
            }
        }
    }
}

void findLinksInURLs()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(urlsMutex);
        cv.wait(lock, []
                { return !urls.empty() || stopCrawling; });
        if (urls.empty())
        {
            lock.unlock();
            break; // Linklerin tamamı tarandığında döngüden çık
        }
        std::string url = urls.front();
        urls.pop();
        lock.unlock();

        CURL *curl;
        curl = curl_easy_init();
        if (curl)
        {
            std::unique_ptr<std::string> html = std::make_unique<std::string>();
            CURLcode res;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, html.get());
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            size_t start = html->find("<a href=\"");
            while (start != std::string::npos)
            {
                size_t end = html->find("\"", start + 9);
                if (end != std::string::npos)
                {
                    std::string link = html->substr(start + 9, end - start - 9);

                    if (link.find("http") != 0)
                    {
                        link = URL_PREFIX + link;
                    }

                    std::lock_guard<std::mutex> urlsLock(urlsMutex);
                    if (std::find(visited.begin(), visited.end(), link) == visited.end())
                    {
                        urls.push(link);
                        cv.notify_one();
                    }
                }
                start = html->find("<a href=\"", end);
            }
        }
    }
}

int main()
{
    urls.push(URL_PREFIX);
    cv.notify_all();

    std::vector<std::thread> threads;
    for (int i = 0; i < MAX_THREADS; i++)
    {
        threads.emplace_back(crawl);
    }

    std::thread linkThread(findLinksInURLs);

    for (auto &t : threads)
    {
        t.join();
    }

    linkThread.join();

    return 0;
}