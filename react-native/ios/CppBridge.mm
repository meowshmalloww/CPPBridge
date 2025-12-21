// =============================================================================
// iOS CppBridge Module
// =============================================================================
// This file is only compiled on iOS/macOS with Xcode.
// The IDE may show errors if React headers are not found - this is expected.
// =============================================================================

#if defined(__APPLE__) && defined(__OBJC__)

#import "CppBridge.h"
#import <React/RCTLog.h>

// Import C++ headers
#include "modules/database/database.h"
#include "modules/database/keyvalue.h"
#include "modules/security/crypto.h"


@implementation CppBridge
RCT_EXPORT_MODULE()

// =============================================================================
// HTTP Methods (Use NSURLSession for iOS - more reliable)
// =============================================================================
RCT_EXPORT_METHOD(httpGet : (NSString *)url resolver : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  NSURL *nsUrl = [NSURL URLWithString:url];
  NSURLRequest *request = [NSURLRequest requestWithURL:nsUrl];

  NSURLSessionDataTask *task = [[NSURLSession sharedSession]
      dataTaskWithRequest:request
        completionHandler:^(NSData *data, NSURLResponse *response,
                            NSError *error) {
          if (error) {
            reject(@"HTTP_ERROR", error.localizedDescription, error);
            return;
          }

          NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
          NSString *body = [[NSString alloc] initWithData:data
                                                 encoding:NSUTF8StringEncoding];

          NSDictionary *result =
              @{@"status" : @(httpResponse.statusCode),
                @"body" : body ?: @""};
          resolve(result);
        }];
  [task resume];
}

RCT_EXPORT_METHOD(httpPost : (NSString *)url body : (NSString *)
                      body contentType : (NSString *)contentType resolver : (
                          RCTPromiseResolveBlock)
                          resolve rejecter : (RCTPromiseRejectBlock)reject) {
  NSURL *nsUrl = [NSURL URLWithString:url];
  NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:nsUrl];
  request.HTTPMethod = @"POST";
  request.HTTPBody = [body dataUsingEncoding:NSUTF8StringEncoding];
  [request setValue:contentType forHTTPHeaderField:@"Content-Type"];

  NSURLSessionDataTask *task = [[NSURLSession sharedSession]
      dataTaskWithRequest:request
        completionHandler:^(NSData *data, NSURLResponse *response,
                            NSError *error) {
          if (error) {
            reject(@"HTTP_ERROR", error.localizedDescription, error);
            return;
          }

          NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
          NSString *responseBody =
              [[NSString alloc] initWithData:data
                                    encoding:NSUTF8StringEncoding];

          NSDictionary *result = @{
            @"status" : @(httpResponse.statusCode),
            @"body" : responseBody ?: @""
          };
          resolve(result);
        }];
  [task resume];
}

// =============================================================================
// Database Methods
// =============================================================================
RCT_EXPORT_METHOD(dbOpen : (NSString *)path resolver : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  int handle = hub_db_open([path UTF8String]);
  if (handle >= 0) {
    resolve(@(handle));
  } else {
    reject(@"DB_ERROR", @"Failed to open database", nil);
  }
}

RCT_EXPORT_METHOD(dbExec : (int)handle sql : (NSString *)sql resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  int result = hub_db_exec(handle, [sql UTF8String]);
  resolve(@(result == 0));
}

RCT_EXPORT_METHOD(dbQuery : (int)handle sql : (NSString *)sql resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  const char *result = hub_db_query(handle, [sql UTF8String]);
  resolve(result ? [NSString stringWithUTF8String:result] : @"[]");
}

RCT_EXPORT_METHOD(dbClose : (int)handle resolver : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  hub_db_close(handle);
  resolve(nil);
}

// =============================================================================
// Key-Value Store Methods
// =============================================================================
RCT_EXPORT_METHOD(kvOpen : (NSString *)path resolver : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  int handle = hub_kv_open([path UTF8String]);
  if (handle >= 0) {
    resolve(@(handle));
  } else {
    reject(@"KV_ERROR", @"Failed to open key-value store", nil);
  }
}

RCT_EXPORT_METHOD(kvSet : (int)handle key : (NSString *)key value : (NSString *)
                      value resolver : (RCTPromiseResolveBlock)
                          resolve rejecter : (RCTPromiseRejectBlock)reject) {
  int result = hub_kv_set(handle, [key UTF8String], [value UTF8String]);
  resolve(@(result == 1));
}

RCT_EXPORT_METHOD(kvGet : (int)handle key : (NSString *)key resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  const char *result = hub_kv_get(handle, [key UTF8String]);
  resolve(result ? [NSString stringWithUTF8String:result] : [NSNull null]);
}

RCT_EXPORT_METHOD(kvDelete : (int)handle key : (NSString *)key resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  int result = hub_kv_delete(handle, [key UTF8String]);
  resolve(@(result == 1));
}

RCT_EXPORT_METHOD(kvClose : (int)handle resolver : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  hub_kv_close(handle);
  resolve(nil);
}

// =============================================================================
// Crypto Methods
// =============================================================================
RCT_EXPORT_METHOD(cryptoSha256 : (NSString *)data resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  const char *dataStr = [data UTF8String];
  unsigned char hash[32];
  hub_crypto_sha256(dataStr, (int)strlen(dataStr), hash, 32);

  NSMutableString *hexString = [NSMutableString stringWithCapacity:64];
  for (int i = 0; i < 32; i++) {
    [hexString appendFormat:@"%02x", hash[i]];
  }
  resolve(hexString);
}

// =============================================================================
// System Methods
// =============================================================================
RCT_EXPORT_METHOD(systemCpuCount : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  resolve(@([[NSProcessInfo processInfo] processorCount]));
}

RCT_EXPORT_METHOD(systemProcessId : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  resolve(@([[NSProcessInfo processInfo] processIdentifier]));
}

@end

#endif // __APPLE__ && __OBJC__
