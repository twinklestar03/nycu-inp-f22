#include "tiny_dns.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <sstream>


QuestionRecord::QuestionRecord(char* serialized) {
    char buffer[512];
    size_t len = 0;
    size_t offset = 0;

    len += TinyDNSd::DecodeName(serialized, buffer);
    qName = buffer;
    offset += len;

    qType = ntohs(*(uint16_t*)(serialized + offset));
    offset += sizeof(uint16_t);
    qClass = ntohs(*(uint16_t*)(serialized + offset));

    std::cout << "QuestionRecord: " << qName << " " << qType << " " << qClass << std::endl;
}

std::string QuestionRecord::Serialize() {
    char buffer[1024];
    size_t offset = 0;
    size_t len = 0;

    len = TinyDNSd::EncodeName(qName.c_str(), buffer);
    offset += len;

    *(uint16_t*)(buffer + offset) = htons(qType);
    offset += sizeof(uint16_t);
    *(uint16_t*)(buffer + offset) = htons(qClass);
    offset += sizeof(uint16_t);

    return std::string(buffer, offset);
}

RDataAAAA::RDataAAAA(std::string data) {
    inet_pton(AF_INET6, data.c_str(), address);
}

RDataAAAA::RDataAAAA(char *serialized) {
    memcpy(address, serialized, 16);
}

std::string RDataAAAA::Serialize() {
    char buffer[INET6_ADDRSTRLEN];
    memcpy(buffer, &address, sizeof(uint32_t) * 4);
    return { buffer, 16 };
}

RDataA::RDataA(std::string data) {
    std::cout << "Parsing A record: " << data << std::endl;
    inet_pton(AF_INET, data.c_str(), &address);
}

RDataA::RDataA(char *serialized) {
    memcpy(&address, serialized, sizeof(uint32_t));
}

std::string RDataA::Serialize() {
    char buffer[INET_ADDRSTRLEN];
    memcpy(buffer, &address, sizeof(uint32_t));
    return { buffer, 4 };
}

RDataNS::RDataNS(std::string data) {
    nsdname = data;
}

RDataNS::RDataNS(char *serialized) {
    char tmp[1024];
    size_t len = TinyDNSd::DecodeName(serialized, tmp);
    nsdname = { tmp, len };
}

std::string RDataNS::Serialize() {
    char buffer[1024];
    size_t offset = TinyDNSd::EncodeName(nsdname.c_str(), buffer);
    return std::string(buffer, offset);
}

RDataCNAME::RDataCNAME(std::string data) {
    cname = data;
}

RDataCNAME::RDataCNAME(char *serialized) {
    char tmp[1024];
    size_t len = TinyDNSd::DecodeName(serialized, tmp);
    cname = { tmp, len };
}

std::string RDataCNAME::Serialize() {
    char buffer[1024];
    size_t offset = TinyDNSd::EncodeName(cname.c_str(), buffer);
    return std::string(buffer, offset);
}

RDataSOA::RDataSOA(std::string config) {
    std::vector<std::string> soa_data;
    std::string token;
    std::istringstream tokenStream(config);
    while (std::getline(tokenStream, token, ' ')) {
        soa_data.push_back(token);
    }

    mname = soa_data[0];
    rname = soa_data[1];
    serial = std::stoi(soa_data[2]);
    refresh = std::stoi(soa_data[3]);
    retry = std::stoi(soa_data[4]);
    expire = std::stoi(soa_data[5]);
    minimum = std::stoi(soa_data[6]);
}

RDataSOA::RDataSOA(char *serialized) {
    char tmp[1024];
    size_t len;
    size_t offset = 0;

    len = TinyDNSd::DecodeName(serialized, tmp);
    mname = { tmp, len };
    offset += len;

    len = TinyDNSd::DecodeName(serialized + offset, tmp);
    rname = { tmp, len };
    offset += len;

    serial = ntohl(*((uint32_t*) (serialized + offset)));
    offset += sizeof(uint32_t);
    refresh = ntohl(*((uint32_t*) (serialized + offset)));
    offset += sizeof(uint32_t);
    retry = ntohl(*((uint32_t*) (serialized + offset)));
    offset += sizeof(uint32_t);
    expire = ntohl(*((uint32_t*) (serialized + offset)));
    offset += sizeof(uint32_t);
    minimum = ntohl(*((uint32_t*) (serialized + offset)));
    offset += sizeof(uint32_t);
}

std::string RDataSOA::Serialize() {
    char buffer[1024];
    size_t offset = 0;
    offset += TinyDNSd::EncodeName(mname.c_str(), buffer);
    offset += TinyDNSd::EncodeName(rname.c_str(), buffer + offset);
    *((uint32_t*) (buffer + offset)) = htonl(serial);
    offset += sizeof(uint32_t);
    *((uint32_t*) (buffer + offset)) = htonl(refresh);
    offset += sizeof(uint32_t);
    *((uint32_t*) (buffer + offset)) = htonl(retry);
    offset += sizeof(uint32_t);
    *((uint32_t*) (buffer + offset)) = htonl(expire);
    offset += sizeof(uint32_t);
    *((uint32_t*) (buffer + offset)) = htonl(minimum);
    offset += sizeof(uint32_t);

    return std::string(buffer, offset);
}

RDataMX::RDataMX(std::string config) {
    std::vector<std::string> mx_data;
    std::string token;
    std::istringstream tokenStream(config);
    while (std::getline(tokenStream, token, ' ')) {
        mx_data.push_back(token);
    }

    preference = std::stoi(mx_data[0]);
    exchange = mx_data[1];
}

RDataMX::RDataMX(char *serialized) {
    char tmp[1024];
    preference = ntohs(*((uint16_t*) serialized));
    TinyDNSd::DecodeName(serialized + sizeof(uint16_t), tmp);
    exchange = tmp;
}

std::string RDataMX::Serialize() {
    char buffer[1024];
    size_t offset = 0;
    *((uint16_t*) (buffer + offset)) = htons(preference);
    offset += sizeof(uint16_t);
    offset += TinyDNSd::EncodeName(exchange.c_str(), buffer + offset);

    return std::string(buffer, offset);
}

RDataTXT::RDataTXT(std::string config) {
    txt_data = config;
}

RDataTXT::RDataTXT(char *serialized) {
    size_t txt_size = serialized[0];
    txt_data = std::string(serialized + 1, txt_size);
}

std::string RDataTXT::Serialize() {
    char buffer[1024];
    size_t offset = 0;
    buffer[offset++] = txt_data.size();
    memcpy(buffer + offset, txt_data.c_str(), txt_data.size());
    offset += txt_data.size();

    return std::string(buffer, offset);
}

std::string ResourceRecord::Serialize() {
    char buffer[2048];
    size_t offset = 0;

    if (rData == nullptr) {
        return "";
    }

    auto serializedRData = rData->Serialize();

    offset += TinyDNSd::EncodeName(rName.c_str(), buffer);
    *((uint16_t*) (buffer + offset)) = htons(rType);
    offset += sizeof(uint16_t);
    *((uint16_t*) (buffer + offset)) = htons(rClass);
    offset += sizeof(uint16_t);
    *((uint32_t*) (buffer + offset)) = htonl(rTTL);
    offset += sizeof(uint32_t);
    *((uint16_t*) (buffer + offset)) = htons(serializedRData.size());
    offset += sizeof(uint16_t);
    memcpy(buffer + offset, serializedRData.data(), serializedRData.size());
    offset += serializedRData.size();

    return std::string(buffer, offset);
}

ResourceRecord ResourceRecord::Parse(char *buffer, size_t offset) {
    ResourceRecord rr;
    char tmp[1024];

    offset += TinyDNSd::DecodeName(buffer, offset, tmp);
    rr.rName = tmp;

    rr.rType = ntohs(*((uint16_t*) (buffer + offset)));
    offset += sizeof(uint16_t);

    rr.rClass = ntohs(*((uint16_t*) (buffer + offset)));
    offset += sizeof(uint16_t);

    rr.rTTL = ntohl(*((uint32_t*) (buffer + offset)));
    offset += sizeof(uint32_t);
    
    rr.rdLength = ntohs(*((uint16_t*) (buffer + offset)));
    offset += sizeof(uint16_t);

    std::cout << "Parsing RR: " << rr.rName << " " << rr.rType << " " << rr.rClass << " " << rr.rTTL << " " << rr.rdLength << std::endl;

    switch (rr.rType) {
        case DNS_TYPE_A:
            rr.rData = std::make_shared<RDataA>(buffer + offset);
            break;
        case DNS_TYPE_AAAA:
            rr.rData = std::make_shared<RDataAAAA>(buffer + offset);
            break;
        case DNS_TYPE_CNAME:
            rr.rData = std::make_shared<RDataCNAME>(buffer + offset);
            break;
        case DNS_TYPE_SOA:
            rr.rData = std::make_shared<RDataSOA>(buffer + offset);
            break;
        case DNS_TYPE_MX:
            rr.rData = std::make_shared<RDataMX>(buffer + offset);
            break;
        case DNS_TYPE_NS:
            rr.rData = std::make_shared<RDataNS>(buffer + offset);
            break;
        case DNS_TYPE_TXT:
            rr.rData = std::make_shared<RDataTXT>(buffer + offset);
            break;
        default:
            rr.rData = std::make_shared<RData>(buffer + offset, rr.rdLength);
            std::cout << "Unknown RR type: " << rr.rType << std::endl;
            break;
    }

    return rr;
}