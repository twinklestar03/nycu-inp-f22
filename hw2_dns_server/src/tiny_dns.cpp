#include "tiny_dns.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <ios>
#include <iostream>
#include <iomanip>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <sys/socket.h>

TinyDNSd::TinyDNSd(char* config_path) {
    ParseConfigFile(config_path);
}

TinyDNSd::~TinyDNSd() {
    close(serverFd);
}

void TinyDNSd::ParseConfigFile(char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (file == nullptr) {
        std::cerr << "[-] [TinyDNSd::ParseConfigFile] Failed to open config file" << std::endl;
        return;
    }

    // Fallback DNS Server IP
    char* line = new char[512];
    fscanf(file, "%s", line);
    fallbackServer = inet_addr(line);

    // Zone files paths
    // Format: example1.org.,zone-example1.org.txt
    while (fscanf(file, "%s", line) != EOF) {
        std::string domain = strtok(line, ",");
        std::string zone_file = strtok(nullptr, ",");
        fprintf(stderr, "[*] [TinyDNSd::ParseConfigFile] Domain: %s, Zone file: %s\n", domain.c_str(), zone_file.c_str());
        
        ParseZoneFile((char*)zone_file.c_str());
    }

    PrintResourceRecords();

    fclose(file);
}

void TinyDNSd::ParseZoneFile(char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (file == nullptr) {
        std::cerr << "[-] [TinyDNSd::ParseZoneFile] Failed to open zone file" << std::endl;
        return;
    }

    char* line = new char[1024];
    // First line is domain name
    fgets(line, 1024, file);
    line[strlen(line) - 2] = '\0'; // remove \r\n

    std::string zone_name = line;
    // remove trailing dot
    if (zone_name[zone_name.length() - 1] == '.') {
        zone_name = zone_name.substr(0, zone_name.length() - 1);
    }

    // read whole line until \n
    while (fgets(line, 1024, file) != nullptr) {
        ResourceRecord record;
        std::string rName;
        std::string rClass;
        std::string rType;
        uint32_t rTTL;
        std::string rdataConfig;

        // if /r/n is present in rdata, remove it
        if (line[strlen(line) - 2] == '\r') {
            line[strlen(line) - 2] = '\0';
        }

        record.config_string = line;

        rName = strtok(line, ",");
        rTTL = atoi(strtok(nullptr, ","));
        rClass = strtok(nullptr, ",");
        rType = strtok(nullptr, ",");
        rdataConfig = strtok(nullptr, ",");

        std::cout << "RData: " << rdataConfig << std::endl;
        
        if (rName == "@") {
            record.rName = strdup(zone_name.c_str());
        } else {
            record.rName = strdup((rName + "." + zone_name).c_str());
        }
        
        record.rTTL = rTTL;
        record.rClass = 1;

        // convert DNS type name into uint16_t
        if (rType == "A") {
            record.rType = DNS_TYPE_A;
            record.rData = std::make_shared<RDataA>(rdataConfig);
            zoneRecords[zone_name][DNS_TYPE_A].push_back(record);
        } else if (rType == "NS") {
            record.rType = DNS_TYPE_NS;
            record.rData = std::make_shared<RDataNS>(rdataConfig);
            zoneRecords[zone_name][DNS_TYPE_NS].push_back(record);
        } else if (rType == "CNAME") {
            record.rType = DNS_TYPE_CNAME;
            record.rData = std::make_shared<RDataCNAME>(rdataConfig);
            zoneRecords[zone_name][DNS_TYPE_CNAME].push_back(record);
        } else if (rType == "SOA") {
            record.rType = DNS_TYPE_SOA;
            record.rData = std::make_shared<RDataSOA>(rdataConfig);
            zoneRecords[zone_name][DNS_TYPE_SOA].push_back(record);
        } else if (rType == "MX") {
            record.rType = DNS_TYPE_MX;
            record.rData = std::make_shared<RDataMX>(rdataConfig);
            zoneRecords[zone_name][DNS_TYPE_MX].push_back(record);
        } else if (rType == "TXT") {
            record.rType = DNS_TYPE_TXT;
            record.rData = std::make_shared<RDataTXT>(rdataConfig);
            zoneRecords[zone_name][DNS_TYPE_TXT].push_back(record);
        } else if (rType == "AAAA") {
            record.rType = DNS_TYPE_AAAA;
            record.rData = std::make_shared<RDataAAAA>(rdataConfig);
            zoneRecords[zone_name][DNS_TYPE_AAAA].push_back(record);
        } else {
            std::cerr << "[-] [TinyDNSd::ParseZoneFile] Unknown DNS type: " << rType << std::endl;
            continue;
        }   
    }

    fclose(file);
}

void TinyDNSd::PrintResourceRecords() {
    for (auto zoneIt = zoneRecords.begin(); zoneIt != zoneRecords.end(); zoneIt++) {
        std::cerr << zoneIt->first << std::endl;
        for (auto typeIt = zoneIt->second.begin(); typeIt != zoneIt->second.end(); typeIt++) {
            for (auto recordIt = typeIt->second.begin(); recordIt != typeIt->second.end(); recordIt++) {
                PrintResourceRecord(*recordIt);
            }
        }
        std::cerr << "----------------" << std::endl;
    }
}

void TinyDNSd::PrintResourceRecord(ResourceRecord& record) {
    std::cerr << ">>>> Config: " << record.config_string << std::endl;
    std::cerr << "Name: " << record.rName << std::endl;
    std::cerr << "TTL: " << record.rTTL << std::endl;
    std::cerr << "Class: " << record.rClass << std::endl;
    std::cerr << "Type: " << record.rType << std::endl;
    
    auto serialized = record.Serialize();
    HexDump(serialized.data(), serialized.size());
}

void TinyDNSd::Run(int port) {
    serverFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverFd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Failed to set socket options" << std::endl;
        return;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        return;
    }

    while (true) {
        Work();
    }
}

void TinyDNSd::Work() {
    // handle only UDP requests
    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    char buffer[1024];
    int bytes = recvfrom(serverFd, buffer, sizeof(buffer), 0, (sockaddr*)&addr, &addrLen);

    if (bytes == -1) {
        std::cerr << "[-] [TinyDNSd::Work] Failed to receive data" << std::endl;
        return;
    }
    std::cerr << "[*] [TinyDNSd::Work] Received " << bytes << " bytes from " << inet_ntoa(addr.sin_addr) << std::endl;

    // parse Packet
    std::unique_ptr<DNSMessage> packet = std::make_unique<DNSMessage>(buffer, bytes);
    // packet = std::move(ParseMessage(buffer, bytes));

    std::cout << "Original packet: " << std::endl;
    HexDump(buffer, bytes);

    std::cout << "Parsed packet: " << std::endl;
    HexDump(packet->Serialize().data(), packet->Serialize().size());

    // handle request
    HandleRequest((sockaddr*) &addr, *packet);
}

void TinyDNSd::HandleRequest(sockaddr* addr, DNSMessage& message) {
    std::cerr << "[*] [TinyDNSd::HandleRequest] Handling request" << std::endl;

    // if it's request
    if ((message.header.flags >> DNS_QR_FIELD) & 1) {
        std::cerr << "[-] [TinyDNSd::HandleRequest] It's not a request" << std::endl;
        return;
    }

    DNSMessage responseMessage = DNSMessage(message);
    auto &question = message.questions[0];
    const std::string& qName = question.qName;
    const uint16_t qType = question.qType;

    // THIS IMPLEMENTATION IS WRONG. Sub-domain query is not supported
    // We should build a record tree to query the record and vise versa
    std::string zone_name;
    std::stringstream ss(qName);
    std::vector<std::string> tokens;
    bool nsPresent = false;
    while (std::getline(ss, zone_name, '.')) {
        tokens.push_back(zone_name);
    }

    // FORMERR
    if (tokens.size() < 2 || message.questions.size() > 1) {
        QueryFallbackNameserver(addr, message);
        return;
    }

    // nip.io like service 
    if (tokens.size() == 7) {
        std::string ip = tokens[0] + "." + tokens[1] + "." + tokens[2] + "." + tokens[3];
        std::string domain = tokens[4] + "." + tokens[5] + "." + tokens[6];

        std::cerr << "[*] [TinyDNSd::HandleRequest] Nip.io like service: " << ip << " " << domain << std::endl;

        responseMessage.header.flags = (1 << DNS_QR_FIELD) | (1 << DNS_AA_FIELD);
        responseMessage.header.answer_count = 1;
        responseMessage.header.authorize_count = 0;

        ResourceRecord record;
        record.rName = qName;
        record.rTTL = 1;
        record.rClass = 1;
        record.rType = 1;
        record.rData = std::make_shared<RDataA>(ip);
        responseMessage.answers.push_back(record);
        SendResponse(addr, responseMessage);
        return;
    }

    zone_name = tokens[tokens.size() - 2] + "." + tokens[tokens.size() - 1];

    // check if we have such zone
    if (zoneRecords.find(zone_name) == zoneRecords.end()) {
        std::cerr << "[/] [TinyDNSd::HandleRequest] Going fallback server. No such zone: " << zone_name << std::endl;
        // TODO: Do fallback server query
        QueryFallbackNameserver(addr, message);
        return;
    }

    // set flags
    responseMessage.header.flags = (1 << DNS_QR_FIELD) | (1 << DNS_AA_FIELD);
    responseMessage.header.answer_count = 0;
    responseMessage.header.authorize_count = 0;

    // check if we have such record
    for (auto &record : zoneRecords[zone_name][qType]) {
        if (record.rName == qName && record.rType == qType) {
            std::cerr << "[*] [TinyDNSd::HandleRequest] Found record" << std::endl;
            responseMessage.header.answer_count++;
            responseMessage.answers.push_back(record);

            // Dealing with additional records
            std::string domain;
            if (record.rType == DNS_TYPE_CNAME || record.rType == DNS_TYPE_NS || record.rType == DNS_TYPE_MX) {
                switch (qType) {
                    case DNS_TYPE_CNAME:
                        domain = reinterpret_cast<RDataCNAME*>(record.rData.get())->cname;
                        domain = domain.substr(0, domain.size() - 1);
                        break;
                    case DNS_TYPE_NS:
                        domain = reinterpret_cast<RDataNS*>(record.rData.get())->nsdname;
                        domain = domain.substr(0, domain.size() - 1);
                        nsPresent = true;
                        break;
                    case DNS_TYPE_MX:
                        domain = reinterpret_cast<RDataMX*>(record.rData.get())->exchange;
                        domain = domain.substr(0, domain.size() - 1);
                        break;
                }

                for (auto &record : zoneRecords[zone_name][DNS_TYPE_A]) {
                    if (record.rName == domain) {
                        std::cerr << "[*] [TinyDNSd::HandleRequest] Found additional record" << std::endl;
                        responseMessage.header.addition_count++;
                        responseMessage.additional.push_back(record);
                    }
                }
            }
        }
    }

    // add authority records
    if (responseMessage.header.answer_count == 0) {
        std::cerr << "[*] [TinyDNSd::HandleRequest] No records found. Adding SOA records" << std::endl;
        responseMessage.header.authorize_count++;
        responseMessage.authorities.push_back(zoneRecords[zone_name][DNS_TYPE_SOA][0]);
    } else if (!nsPresent) {
        std::cerr << "[*] [TinyDNSd::HandleRequest] Records found. Adding NS records" << std::endl;
        responseMessage.header.authorize_count++;
        responseMessage.authorities.push_back(zoneRecords[zone_name][DNS_TYPE_NS][0]);
    }

    // send response
    SendResponse(addr, responseMessage);
}

void TinyDNSd::SendResponse(sockaddr* addr, DNSMessage& packet) {
    std::cerr << "[*] [TinyDNSd::SendResponse] Sending response" << std::endl;
    size_t rawSize = 0;
    std::string serialized = packet.Serialize();
    int bytes = sendto(serverFd, serialized.data(), serialized.size(), 0, addr, sizeof(sockaddr_in));
    if (bytes == -1) {
        std::cerr << "[-] [TinyDNSd::SendResponse] Failed to send response" << std::endl;
    }

    std::cerr << "[*] [TinyDNSd::SendResponse] Sent " << bytes << " bytes" << std::endl;
}

void TinyDNSd::QueryFallbackNameserver(sockaddr* clientAddr, DNSMessage &message) {
    int sockFd;
    char buffer[1024];
    size_t bytes;
    sockaddr_in serverAddr;
    std::string serialized;
    
    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd == -1) {
        std::cerr << "[-] [TinyDNSd::QueryFallbackNameserver] Failed to create socket" << std::endl;
        return;
    }

    memset(&serverAddr, 0, sizeof(sockaddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(53);
    serverAddr.sin_addr.s_addr = fallbackServer;

    serialized = message.Serialize();
    bytes = sendto(sockFd, serialized.data(), serialized.size(), 0, (sockaddr*)&serverAddr, sizeof(sockaddr_in));
    if (bytes == -1) {
        std::cerr << "[-] [TinyDNSd::QueryFallbackNameserver] Failed to send request" << std::endl;
        return;
    }

    std::cerr << "[*] [TinyDNSd::QueryFallbackNameserver] Sent " << bytes << " bytes" << std::endl;

    bytes = recvfrom(sockFd, buffer, 1024, 0, NULL, NULL);
    if (bytes == -1) {
        std::cerr << "[-] [TinyDNSd::QueryFallbackNameserver] Failed to receive response" << std::endl;
        return;
    }

    std::cerr << "[*] [TinyDNSd::QueryFallbackNameserver] Received " << bytes << " bytes" << std::endl;
    sendto(serverFd, buffer, bytes, 0, clientAddr, sizeof(sockaddr_in));

    return;
}

size_t TinyDNSd::DecodeName(const char* src, char* dst) {
    int8_t part_len = src[0];
    size_t offset = 0;
    std::string name = "";

    while (part_len != 0) {
        name += std::string(src + offset + 1, part_len);
        offset += part_len + 1;
        part_len = src[offset];
        if (part_len != 0) {
            name += ".";
        }
    }

    strcpy(dst, name.c_str());
    return offset + 1;
}

size_t TinyDNSd::DecodeName(const char* src, size_t offset, char* dst) {
    int8_t part_len = src[offset];

    // if first two bit are set, then this is a pointer
    if ((part_len & 0xC0) == 0xC0) {
        uint16_t pointer = (part_len & 0x3F) << 8 | src[offset + 1];
        DecodeName(src + pointer, dst);
        return 2;
    }

    return DecodeName(src + offset, dst);
}

size_t TinyDNSd::EncodeName(const char* src, char* dst) {
    size_t offset = 0;
    std::string name = src;
    std::vector<std::string> parts = SplitString(name, '.');

    for (auto part : parts) {
        dst[offset] = part.size();
        memcpy(dst + offset + 1, part.c_str(), part.size());
        offset += part.size() + 1;
    }
    dst[offset] = 0;

    return offset + 1;
}

std::vector<std::string> TinyDNSd::SplitString(std::string str, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

void TinyDNSd::HexDump(const char *buffer, size_t size) {
    // Backup cerr format
    std::ios init(NULL);
    init.copyfmt(std::cerr);
    // hexdump that has offset and ascii representation
    size_t alignedSize = size + (16 - size % 16);
    for (size_t i = 0; i < alignedSize; i++) {
        if (i % 16 == 0) {
            std::cerr << std::hex << std::setw(4) << std::setfill('0') << i << " ";
        }
        if (i < size){
            std::cerr << std::hex << std::setw(2) << std::setfill('0') << ((uint8_t)buffer[i] & 0xff) << " ";
        } else {
            std::cerr << "   ";
        }

        if (i % 16 == 15) {
            std::cerr << " ";
            for (size_t j = i - 15; j <= i; j++) {
                if (j >= size) {
                    std::cerr << "-";
                } else if (isprint(buffer[j])) {
                    std::cerr << buffer[j];
                } else {
                    std::cerr << ".";
                }
            }
            std::cerr << std::endl;
        }
    }
    std::cerr << std::endl;

    // Restore cerr format
    std::cerr.copyfmt(init);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config>" << std::endl;
        return 1;
    }

    TinyDNSd dns(argv[1]);
    dns.Run(53);

    return 0;
}