#include "gtest/gtest.h"

#include <pistache/cookie.h>
#include <pistache/date.h>

using namespace Pistache;
using namespace Pistache::Http;

void parse(const char* str, std::function<void (const Cookie&)> testFunc)
{
    auto c1 = Cookie::fromString(std::string(str));
    testFunc(c1);

    auto c2 = Cookie::fromRaw(str, strlen(str));
    testFunc(c2);
}

TEST(cookie_test, basic_test) {
    parse("SID=31d4d96e407aad42", [](const Cookie& cookie) {
        ASSERT_EQ(cookie.name, "SID");
        ASSERT_EQ(cookie.value, "31d4d96e407aad42");
    });
}

TEST(cookie_test, attributes_test) {
    parse("SID=31d4d96e407aad42; Path=/", [](const Cookie& c) {
        ASSERT_EQ(c.name, "SID");
        ASSERT_EQ(c.value, "31d4d96e407aad42");
        ASSERT_EQ(c.path.getOrElse(""), "/");
    });

    parse("SID=31d4d96e407aad42; Path=/; Domain=example.com", [](const Cookie& c) {
        ASSERT_EQ(c.path.getOrElse(""), "/");
        ASSERT_EQ(c.domain.getOrElse(""), "example.com");
    });

    parse("lang=en-US; Path=/; Domain=example.com; Max-Age=10", [](const Cookie& c) {
        ASSERT_EQ(c.name, "lang");
        ASSERT_EQ(c.value, "en-US");
        ASSERT_EQ(c.path.getOrElse(""), "/");
        ASSERT_EQ(c.domain.getOrElse(""), "example.com");
        ASSERT_EQ(c.maxAge.getOrElse(0), 10);
    });

    parse("lang=en-US; Expires=Wed, 09 Jun 2021 10:18:14 GMT", [](const Cookie& c) {
        ASSERT_EQ(c.name, "lang");
        ASSERT_EQ(c.value, "en-US");
        auto expires = c.expires.getOrElse(FullDate());
        auto date = expires.date();
        
        using namespace std::chrono;
        FullDate::time_point expected_time_point = date::sys_days(date::year{2021}/6/9) 
                                                    + hours(10) + minutes(18) + seconds(14);
        ASSERT_EQ(date, expected_time_point);
    });

    parse("lang=en-US; Path=/; Domain=example.com;", [](const Cookie& c) {
        ASSERT_EQ(c.name, "lang");
        ASSERT_EQ(c.value, "en-US");
        ASSERT_EQ(c.domain.getOrElse(""), "example.com");
    });
}

TEST(cookie_test, bool_test) {
    parse("SID=31d4d96e407aad42; Path=/; Secure", [](const Cookie& c) {
        ASSERT_EQ(c.name, "SID");
        ASSERT_EQ(c.value, "31d4d96e407aad42");
        ASSERT_EQ(c.path.getOrElse(""), "/");
        ASSERT_TRUE(c.secure);
        ASSERT_FALSE(c.httpOnly);
    });

    parse("SID=31d4d96e407aad42; Path=/; Secure; HttpOnly", [](const Cookie& c) {
        ASSERT_EQ(c.name, "SID");
        ASSERT_EQ(c.value, "31d4d96e407aad42");
        ASSERT_EQ(c.path.getOrElse(""), "/");
        ASSERT_TRUE(c.secure);
        ASSERT_TRUE(c.httpOnly);
    });

}

TEST(cookie_test, ext_test) {
    parse("lang=en-US; Path=/; Scope=Private", [](const Cookie& c) {
        ASSERT_EQ(c.name, "lang");
        ASSERT_EQ(c.value, "en-US");
        ASSERT_EQ(c.path.getOrElse(""), "/");
        auto fooIt = c.ext.find("Scope");
        ASSERT_TRUE(fooIt != std::end(c.ext));
        ASSERT_EQ(fooIt->second, "Private");
    });
}

TEST(cookie_test, write_test) {
    Cookie c1("lang", "fr-FR");
    c1.path = Some(std::string("/"));
    c1.domain = Some(std::string("example.com"));

    std::ostringstream oss;
    c1.write(oss);

    ASSERT_EQ(oss.str(), "lang=fr-FR; Path=/; Domain=example.com");

    Cookie c2("lang", "en-US");
    using namespace std::chrono;
    
    FullDate::time_point expires = date::sys_days(date::year{118}/2/16) + hours(17);
    
    c2.path = Some(std::string("/"));
    c2.expires = Some(FullDate(expires));

    oss.str("");
    c2.write(oss);

    Cookie c3("lang", "en-US");
    c3.secure = true;
    c3.ext.insert(std::make_pair("Scope", "Private"));
    oss.str("");
    c3.write(oss);

    ASSERT_EQ(oss.str(), "lang=en-US; Secure; Scope=Private");
};

TEST(cookie_test, invalid_test) {
    ASSERT_THROW(Cookie::fromString("lang"), std::runtime_error);
    ASSERT_THROW(Cookie::fromString("lang=en-US; Expires"), std::runtime_error);
    ASSERT_THROW(Cookie::fromString("lang=en-US; Path=/; Domain"), std::runtime_error);

    ASSERT_THROW(Cookie::fromString("lang=en-US; Max-Age=12ab"), std::invalid_argument);
}

