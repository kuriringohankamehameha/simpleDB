#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <iostream>
#include <cstdint>
#include <memory>
#include <string>
#include <json/json.h>

using namespace std;

//Compilation:
// curl-config --cflags

//Linking with libcurl:
// curl-config --libs

//To check for SSL support:
// curl-config --feature

//Run with Makefile

/* Easy Interface:
libcurl first introduced the so called easy interface.
All operations in the easy interface are prefixed with 'curl_easy'.
The easy interface lets you do single transfers with a synchronous and blocking function call.
*/

size_t write_data(void* buffer_size, size_t size, size_t nmemb, void* userp)
{
	//Get data from a remote Application to come here
	return size;
}

namespace
{
    std::size_t callback(
            const char* in,
            std::size_t size,
            std::size_t num,
            std::string* out)
    {
        const std::size_t totalBytes(size * num);
        out->append(in, totalBytes);
        return totalBytes;
    }
}

int main()
{
	cout << "A Server using libcurl\n";

	CURL* easyhandle;

	CURLcode res;

	easyhandle = curl_easy_init();

	//Display a lot of verbose information 
	long onoff = 1;

	//Set verbose output on the handle
	curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, onoff);

	//Provide a URL to use in the request
	curl_easy_setopt(easyhandle, CURLOPT_URL, "http://google.com");

	//Pass all data to write_data()
	//curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, write_data);
	CURL *curl_z;
	CURLcode res_2;

	curl_z = curl_easy_init();
	if (curl_z) {
		curl_easy_setopt(curl_z, CURLOPT_URL, "http://google.com/");
		res_2 = curl_easy_perform(curl_z);
		cout << res_2 << endl;
		curl_easy_cleanup(curl_z);
	}

	cout << "Now fetching json objects from a url.\n";

	 const std::string url("http://date.jsontest.com/");

    CURL* curl = curl_easy_init();

    // Set remote URL.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Response information.
    int httpCode(0);
    std::unique_ptr<std::string> httpData(new std::string());

    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

    // Hook up data container (will be passed as the last parameter to the
    // callback handling function).  Can be any pointer type, since it will
    // internally be passed as a void pointer.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());

    // Run our HTTP GET command, capture the HTTP response code, and clean up.
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode == 200)
    {
        std::cout << "\nGot successful response from " << url << std::endl;

        // Response looks good - done using Curl now.  Try to parse the results
        // and print them out.
        Json::Value jsonData;
        Json::Reader jsonReader;

        if (jsonReader.parse(*httpData, jsonData))
        {
            std::cout << "Successfully parsed JSON data" << std::endl;
            std::cout << "\nJSON data received:" << std::endl;
            std::cout << jsonData.toStyledString() << std::endl;

            const std::string dateString(jsonData["date"].asString());
            const std::size_t unixTimeMs(
                    jsonData["milliseconds_since_epoch"].asUInt64());
            const std::string timeString(jsonData["time"].asString());

            std::cout << "Natively parsed:" << std::endl;
            std::cout << "\tDate string: " << dateString << std::endl;
            std::cout << "\tUnix timeMs: " << unixTimeMs << std::endl;
            std::cout << "\tTime string: " << timeString << std::endl;
            std::cout << std::endl;
        }
        else
        {
            std::cout << "Could not parse HTTP data as JSON" << std::endl;
            std::cout << "HTTP data was:\n" << *httpData.get() << std::endl;
            return 1;
        }
    }
    else
    {
        std::cout << "Couldn't GET from " << url << " - exiting" << std::endl;
        return 1;
    }

	return 0;
}