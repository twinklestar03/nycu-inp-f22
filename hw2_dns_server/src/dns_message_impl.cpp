#include "tiny_dns.h"


DNSMessage::DNSMessage(char* buffer, size_t size) {
    header = DNSHeader(buffer);
    size_t offset = sizeof(DNSHeader);

    for (int i = 0; i < header.question_count; i++) {
        questions.push_back(QuestionRecord(buffer + offset));
        offset += questions.back().Serialize().size();
    }
    for (int i = 0; i < header.answer_count; i++) {
        answers.push_back(ResourceRecord::Parse(buffer, offset));
        offset += answers.back().Serialize().size();
    }
    for (int i = 0; i < header.authorize_count; i++) {
        authorities.push_back(ResourceRecord::Parse(buffer, offset));
        offset += authorities.back().Serialize().size();
    }
    for (int i = 0; i < header.addition_count; i++) {
        additional.push_back(ResourceRecord::Parse(buffer, offset));
        offset += additional.back().Serialize().size();
    }
}

DNSMessage::DNSMessage(const DNSMessage& other) {
    memcpy(&header, &other.header, sizeof(DNSHeader));
    questions = other.questions;
    answers = other.answers;
    authorities = other.authorities;
    additional = other.additional;
}

std::string DNSMessage::Serialize() {
    char buffer[1024];
    size_t offset = 0;

    memcpy(buffer, header.Serialize().data(), 12);
    offset += 12;

    for (auto& question : questions) {
        std::string serialized = question.Serialize();
        memcpy(buffer + offset, serialized.c_str(), serialized.size());
        offset += serialized.size();
    }

    for (auto& answer : answers) {
        std::string serialized = answer.Serialize();
        memcpy(buffer + offset, serialized.c_str(), serialized.size());
        offset += serialized.size();
    }
    for (auto& authority : authorities) {
        std::string serialized = authority.Serialize();
        memcpy(buffer + offset, serialized.c_str(), serialized.size());
        offset += serialized.size();
    }
    for (auto& addition : additional) {
        std::string serialized = addition.Serialize();
        memcpy(buffer + offset, serialized.c_str(), serialized.size());
        offset += serialized.size();
    }

    return std::string(buffer, offset);
}

DNSHeader::DNSHeader(char* buffer) {
    id = ntohs(*(uint16_t*)buffer);
    flags = ntohs(*(uint16_t*)(buffer + 2));
    question_count = ntohs(*(uint16_t*)(buffer + 4));
    answer_count = ntohs(*(uint16_t*)(buffer + 6));
    authorize_count = ntohs(*(uint16_t*)(buffer + 8));
    addition_count = ntohs(*(uint16_t*)(buffer + 10));
}

std::string DNSHeader::Serialize() {
    char buffer[12];
    *(uint16_t*)buffer = htons(id);
    *(uint16_t*)(buffer + 2) = htons(flags);
    *(uint16_t*)(buffer + 4) = htons(question_count);
    *(uint16_t*)(buffer + 6) = htons(answer_count);
    *(uint16_t*)(buffer + 8) = htons(authorize_count);
    *(uint16_t*)(buffer + 10) = htons(addition_count);
    return std::string(buffer, 12);
}