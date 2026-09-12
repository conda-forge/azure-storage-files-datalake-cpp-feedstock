#include <azure/storage/files/datalake/datalake_file_client.hpp>
#include <azure/core/http/transport.hpp>
#include <azure/core/io/body_stream.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
class MockTransport final : public Azure::Core::Http::HttpTransport {
public:
    int calls = 0;
    std::unique_ptr<Azure::Core::Http::RawResponse> Send(
        Azure::Core::Http::Request& request, const Azure::Core::Context&) override {
        if (request.GetUrl().GetPath() != "container/blob" ||
            request.GetUrl().GetHost() != "account.blob.core.windows.net")
            throw std::runtime_error("Unexpected blob path");
        ++calls;
        auto response = std::make_unique<Azure::Core::Http::RawResponse>(
            1, 1, Azure::Core::Http::HttpStatusCode::Ok, "OK");
        static const std::string body = "native-arm64-datalake";
        response->SetHeader("Content-Length", std::to_string(body.size()));
        response->SetHeader("Content-Type", "application/octet-stream");
        response->SetHeader("ETag", "\"test-etag\"");
        response->SetHeader("Last-Modified", "Sat, 12 Sep 2026 00:00:00 GMT");
        response->SetHeader("x-ms-creation-time", "Sat, 12 Sep 2026 00:00:00 GMT");
        response->SetHeader("x-ms-blob-type", "BlockBlob");
        response->SetHeader("x-ms-server-encrypted", "true");
        response->SetBodyStream(std::make_unique<Azure::Core::IO::MemoryBodyStream>(
            reinterpret_cast<const uint8_t*>(body.data()), body.size()));
        return response;
    }
};
int main() {
    auto transport = std::make_shared<MockTransport>();
    Azure::Storage::Files::DataLake::DataLakeClientOptions options;
    options.Transport.Transport = transport;
    Azure::Storage::Files::DataLake::DataLakeFileClient client(
        "https://account.dfs.core.windows.net/container/blob", options);
    auto response = client.Download();
    auto bytes = response.Value.Body->ReadToEnd();
    if (std::string(bytes.begin(), bytes.end()) != "native-arm64-datalake" ||
        transport->calls != 1) return 1;
    std::cout << "Installed Azure Data Lake mocked download passed\n";
}
