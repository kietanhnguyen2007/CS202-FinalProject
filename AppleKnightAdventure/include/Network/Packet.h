#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <string>
#include <cstdint>

class Packet {
protected:
    std::vector<char> m_buffer;
    size_t m_readPos;

public:
    Packet();
    explicit Packet(const std::vector<char>& data);

    void AppendInt(int32_t value);
    void AppendFloat(float value);
    void AppendBool(bool value);
    void AppendString(const std::string& value);

    int32_t ReadInt();
    float ReadFloat();
    bool ReadBool();
    std::string ReadString();

    const char* GetData() const;
    char* GetData();
    size_t GetSize() const;
    size_t GetReadPos() const;
    void SetReadPos(size_t pos);
    bool IsEmpty() const;
    void Clear();
    void AppendRaw(const void* data, size_t size);
    void Read(void* data, size_t size);
};

#endif
