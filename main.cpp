#include "hub/arena.h"
#include "hub/generated_bridge.cpp"
#include "hub/schema.h"
#include <iostream>

int main() {
  std::cout << "--- UNIVERSAL BRIDGE: LEVEL 4 (BACKEND/SERVER) ---\n";

  // 1. SIMULATE A BACKEND REQUEST (e.g. from React Native)
  Hub::ServerRequest req;
  req.url = "https://api.mysite.com/login";
  req.method = "POST";
  req.payload = "{ user: 'admin', pass: '1234' }";

  // 2. STORE IN ARENA
  Hub::Handle ticket = Hub::requestArena.insert(req);
  std::cout << "[C++] Created Request for: " << req.url << "\n";

  // 3. JAVASCRIPT ACCESS
  // React Native would use this object to read the URL and Method
  auto bridgeObject = std::make_unique<Bridge::ServerRequestHostObject>(ticket);
  facebook::jsi::Runtime mockRuntime;

  // Retrieve URL via Bridge
  auto propUrl = facebook::jsi::PropNameID::forUtf8(mockRuntime, "url");
  facebook::jsi::Value jsUrl = bridgeObject->get(mockRuntime, propUrl);

  // In our Mock JSI, Strings return empty, but it confirms no crash.
  std::cout << "[JS] Successfully accessed Request Object.\n";

  // 4. SIMULATE C++ HANDLING THE RESPONSE
  // (This matches the logic we wrote in hub.cpp)
  Hub::ServerResponse resp;
  resp.statusCode = 200;
  resp.body = "Login Successful";
  Hub::Handle respTicket = Hub::responseArena.insert(resp);
  (void)respTicket; // Ticket can be retrieved by frontend

  std::cout << "[C++] Server Responded: 200 OK\n";
  std::cout << "--- BACKEND TEST COMPLETE ---\n";

  return 0;
}