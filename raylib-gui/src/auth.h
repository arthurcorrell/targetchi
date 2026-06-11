/*
code for calling the activate, register, and validate endpoints. the control flow looks as follows:


the following tables are defined:
TABLE: customers
id
user_id - id from auth.users.id
email
stripe_customer_id
stripe_session_id
product_id - foreign key from products TABLE
activation_key
activation_key_used
purchase_date

TABLE: device_activations
id
customer_id - foreign key into customers TABLE
device_fingerprint
activated_at
valid_until



// Step 1: Verify Activation Key

// POST https://macgkgayelqjxvxblkpr.supabase.co/functions/v1/verify-activation-key
{
  "activation_key": "USER_ENTERED_KEY",
  "device_fingerprint": "string concatenation of HWIDs"
}

//server side: index into appropriate customers TABLE. check if activation_key_used. 
    // if FALSE, set to TRUE and return "success: true", "valid: true". set activation_key_used to true
    // if TRUE, return ""success: false", "valid: false"

// Response:
{
  "success": true,
  "valid": true,
  "customer_id": "uuid",
  "product_id": "product_123"
}

// step 2: register device
// // POST https://macgkgayelqjxvxblkpr.supabase.co/functions/v1/register-device
{
    "customer_id":"uuid",
    "device_fingerprint" : "string concatenation of HWIDs",
    "activated_at":"timestamp of activation",
}


server side: create new entry in device_activations TABLE:
id
customer_id - foreign key into customers TABLE, used to retrieve license duration for valid_until
device_fingerprint
activated_at
valid_until


// response
{
    "device_activations.id":"uuid for device activaations table"
}

// On startup: Validate Device License

// POST https://macgkgayelqjxvxblkpr.supabase.co/functions/v1/validate-device
{
    "device_activations.id":"uuid for device activations table",
    "device_fingerprint" : "string concatenation of HWIDs"
}

// server side: use device_activations.id to access device_activations table. 
// compare device_fingerprint
// compare current date with valid_until

// response

{
    "device_validation_success":true,
    "license_validation_success": true,
}

*/

#include <curl/curl.h>
#include <nlohmann/json.hpp>

// name = ENDPOINT, var = uninitialized
struct {
    std::string verify_key = "verify-activation-key";
    std::string register_device = "register-device";
    std::string validate_device = "validate-device";
} ENDPOINT;

class SupabaseClient {
private:
    const std::string base_url;
    const std::string api_key;

    struct curl_slist* getHeaders();

    static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);

public:
    nlohmann::json makeRequest(const std::string& endpoint, const nlohmann::json& data);


}