#include <iostream>
#include "auth.h"

int main() {
    SupabaseClient client;

    // Verify activation key
    nlohmann::json verify_request = {
        {"activation_key", "3066D636B2E95C9B0E8EC9BDFF9BB869"},
        {"device_fingerprint", "bruh"}
    };

    auto response = client.makeRequest(ENDPOINT.verify_key, verify_request);
    std::cout << response.dump() << std::endl;

    return 0;

}