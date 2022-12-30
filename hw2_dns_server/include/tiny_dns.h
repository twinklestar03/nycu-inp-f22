#pragma once
#include <cstdint>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#define DNS_PORT 53

#define DNS_QR_FIELD 15
#define DNS_OPCODE_FIELD 11
#define DNS_AA_FIELD 10
#define DNS_TC_FIELD 9
#define DNS_RD_FIELD 8
#define DNS_RA_FIELD 7
#define DNS_RCODE_FIELD 0

#define DNS_TYPE_A 1
#define DNS_TYPE_NS 2
#define DNS_TYPE_CNAME 5
#define DNS_TYPE_SOA 6
#define DNS_TYPE_MX 15
#define DNS_TYPE_TXT 16
#define DNS_TYPE_AAAA 28


struct QuestionRecord {
    QuestionRecord() = default;
    QuestionRecord(char* serialized);
    std::string Serialize();

    std::string qName;
    uint16_t qType;
    uint16_t qClass;
};

struct RData {
    RData() = default;
    RData(char* serialized, size_t size) { rData = {serialized, size}; };
    virtual std::string Serialize() { return rData; };

    std::string rData;
};

struct RDataA : public RData {
    RDataA(std::string config);
    RDataA(char* serialized);
    std::string Serialize() override;

    uint32_t address;
};

struct RDataAAAA : public RData {
    RDataAAAA(std::string config);
    RDataAAAA(char* serialized);
    std::string Serialize() override;

    char address[16];
};

struct RDataNS : public RData {
    RDataNS(std::string config);
    RDataNS(char* serialized);
    std::string Serialize() override;

    std::string nsdname;
};

struct RDataCNAME : public RData {
    RDataCNAME(std::string config);
    RDataCNAME(char* serialized);
    std::string Serialize() override;

    std::string cname;
};

struct RDataSOA : public RData {
    RDataSOA(std::string config);
    RDataSOA(char* serialized);
    std::string Serialize() override;

    std::string mname;
    std::string rname;
    uint32_t serial;
    uint32_t refresh;
    uint32_t retry;
    uint32_t expire;
    uint32_t minimum;
};

struct RDataMX : public RData {
    RDataMX(std::string config);
    RDataMX(char* serialized);
    std::string Serialize() override;

    uint16_t preference;
    std::string exchange;
};

struct RDataTXT : public RData {
    RDataTXT(std::string config);
    RDataTXT(char* serialized);
    std::string Serialize() override;

    std::string txt_data;
};

struct ResourceRecord {
    ResourceRecord() = default;
    static ResourceRecord Parse(char* buffer, size_t offset);
    std::string Serialize();

    std::string rName;
    uint16_t rType;
    uint16_t rClass;
    uint32_t rTTL;
    uint16_t rdLength;
    std::shared_ptr<RData> rData;
    std::string config_string;
};

struct DNSHeader {
    DNSHeader() = default;
    DNSHeader(char* buffer);
    std::string Serialize();

    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_count;
    uint16_t authorize_count;
    uint16_t addition_count;
};

struct DNSMessage {
    DNSMessage() = default;
    DNSMessage(char* buffer, size_t size);
    DNSMessage(const DNSMessage& other);
    std::string Serialize();

    DNSHeader header;
    std::vector<QuestionRecord> questions;
    std::vector<ResourceRecord> answers;
    std::vector<ResourceRecord> authorities;
    std::vector<ResourceRecord> additional;
};

class TinyDNSd {
public:
    TinyDNSd(char* config_path);
    ~TinyDNSd();

    void Run(int port);

    /* Protocol */
    void Work();
    void HandleRequest(sockaddr* addr, DNSMessage& message);
    void SendResponse(sockaddr* addr, DNSMessage& message);
    void QueryFallbackNameserver(sockaddr* addr, DNSMessage& message);

    /* Configuration Parsing */
    void ParseConfigFile(char* file_path);
    void ParseZoneFile(char* file_path);
    
    /* Logging Utils */
    void PrintResourceRecord(ResourceRecord& rr);
    void PrintResourceRecords();
    void PrintHeader(DNSHeader& header);

    /* Helpers */
    // return comsumed size
    static size_t DecodeName(const char* src, char* dst);
    // the decode function that can decompress the name, return comsumed size
    static size_t DecodeName(const char* src, size_t offset, char* dst);
    // return the encoded name size
    static size_t EncodeName(const char* src, char* dst);
    static std::vector<std::string> SplitString(std::string str, char delim);
    static void HexDump(const char* buffer, size_t size);

private:
    int serverFd;
    std::vector<int> clients;

    // zoneName -> (recordType -> ResourceRecord)
    std::map<std::string, std::map<uint16_t, std::vector<ResourceRecord>>> zoneRecords;
    //std::map<std::string, std::vector<ResourceRecord>> zoneRecords;

    uint32_t fallbackServer;
};