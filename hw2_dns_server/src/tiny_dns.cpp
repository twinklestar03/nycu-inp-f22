#include "tiny_dns.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <memory>
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
        std::string name;
        std::string cls;
        std::string type;
        uint32_t ttl;
        char* rdata;

        // if /r/n is present in rdata, remove it
        if (line[strlen(line) - 2] == '\r') {
            line[strlen(line) - 2] = '\0';
        }

        record.config_string = line;

        name = strtok(line, ",");
        ttl = atoi(strtok(nullptr, ","));
        cls = strtok(nullptr, ",");
        type = strtok(nullptr, ",");
        rdata = strtok(nullptr, ",");

        std::cout << "RData: " << rdata << std::endl;
        
        if (name == "@") {
            record.rName = strdup(zone_name.c_str());
        } else {
            record.rName = strdup((name + "." + zone_name).c_str());
        }
        
        record.rTTL = ttl;
        record.rClass = 1;

        // convert DNS type name into uint16_t
        if (type == "A") {
            record.rType = DNS_TYPE_A;
            record.rData = std::make_shared<RDataA>(rdata);
        } else if (type == "NS") {
            record.rType = DNS_TYPE_NS;
            record.rData = std::make_shared<RDataNS>(rdata);
        } else if (type == "CNAME") {
            record.rType = DNS_TYPE_CNAME;
            record.rData = std::make_shared<RDataCNAME>(rdata);
        } else if (type == "SOA") {
            record.rType = DNS_TYPE_SOA;
            record.rData = std::make_shared<RDataSOA>(rdata);
        } else if (type == "MX") {
            record.rType = DNS_TYPE_MX;
            record.rData = std::make_shared<RDataMX>(rdata);
        } else if (type == "TXT") {
            record.rType = DNS_TYPE_TXT;
            record.rData = std::make_shared<RDataTXT>(rdata);
        } else if (type == "AAAA") {
            record.rType = DNS_TYPE_AAAA;
            record.rData = std::make_shared<RDataAAAA>(rdata);
        } else {
            std::cerr << "[-] [TinyDNSd::ParseZoneFile] Unknown DNS type: " << type << std::endl;
            continue;
        }
        
        zoneRecords[zone_name].push_back(record);
    }

    fclose(file);
}

void TinyDNSd::PrintResourceRecords() {
    for (auto it = zoneRecords.begin(); it != zoneRecords.end(); it++) {
        std::cerr << it->first << std::endl;
        for (auto &record : it->second) {
            PrintResourceRecord(record);
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

void TinyDNSd::HandleRequest(sockaddr* addr, DNSMessage& packet) {
    
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