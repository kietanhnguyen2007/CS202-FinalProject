#include "Network/Packet.h"
#include <cstring>

Packet::Packet()
    : m_readPos(0)
{
    m_buffer.reserve(1024);
}

Packet::Packet(const std::vector<char>& data)
    : m_buffer(data)
    , m_readPos(0)
{
}

void Packet::AppendInt(int32_t value) {
    AppendRaw(&value, sizeof(value));
}

void Packet::AppendFloat(float value) {
    AppendRaw(&value, sizeof(value));
}

void Packet::AppendBool(bool value) {
    char byte = value ? 1 : 0;
    AppendRaw(&byte, sizeof(byte));
}

void Packet::AppendString(const std::string& value) {
    uint32_t length = static_cast<uint32_t>(value.size());
    AppendRaw(&length, sizeof(length));
    AppendRaw(value.data(), length);
}

int32_t Packet::ReadInt() {
    int32_t value = 0;
    Read(&value, sizeof(value));
    return value;
}

float Packet::ReadFloat() {
    float value = 0.0f;
    Read(&value, sizeof(value));
    return value;
}

bool Packet::ReadBool() {
    char byte = 0;
    Read(&byte, sizeof(byte));
    return byte != 0;
}

std::string Packet::ReadString() {
    uint32_t length = ReadInt();
    if (length > 0 && m_readPos + length <= m_buffer.size()) {
        std::string value(m_buffer.data() + m_readPos, length);
        m_readPos += length;
        return value;
    }
    return {};
}

const char* Packet::GetData() const {
    return m_buffer.data();
}

char* Packet::GetData() {
    return m_buffer.data();
}

size_t Packet::GetSize() const {
    return m_buffer.size();
}

size_t Packet::GetReadPos() const {
    return m_readPos;
}

void Packet::SetReadPos(size_t pos) {
    m_readPos = pos;
}

bool Packet::IsEmpty() const {
    return m_buffer.empty();
}

void Packet::Clear() {
    m_buffer.clear();
    m_readPos = 0;
}

void Packet::AppendRaw(const void* data, size_t size) {
    const char* bytes = static_cast<const char*>(data);
    m_buffer.insert(m_buffer.end(), bytes, bytes + size);
}

void Packet::Read(void* data, size_t size) {
    if (m_readPos + size <= m_buffer.size()) {
        std::memcpy(data, m_buffer.data() + m_readPos, size);
        m_readPos += size;
    }
}
