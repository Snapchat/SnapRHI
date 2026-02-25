#include "snap/rhi/backend/opengl/PipelineCacheUtils.hpp"

#include "snap/rhi/common/Throw.h"
#include <cassert>
#include <cstring>
#include <string>
#include <variant>
#include <vector>

namespace {
enum class CacheHashType : uint32_t {
    FullSource = 0,
    Hash64,

    Count
};

struct BinCacheFileElement {
    CacheHashType hashType = CacheHashType::Hash64;
    std::variant<snap::rhi::backend::opengl::hash64, std::string> key;

    GLenum format = GL_NONE;
    std::vector<GLubyte> binData{};
};

struct BinCacheFile {
    uint32_t version = 0;
    std::vector<BinCacheFileElement> elements{};
};

constexpr uint32_t VERSION = 100;

uint32_t roundSize(const size_t size) {
    return ((size + 3) >> 2) << 2;
}

template<typename UIntX>
void serializeUIntX(std::vector<uint8_t>& dst, uint32_t& offset, const UIntX src) {
    const size_t size = offset + sizeof(UIntX);
    if (dst.size() < size) {
        dst.resize(size);
    }

    uint8_t* ptr = dst.data() + offset;
    UIntX* ptrX = reinterpret_cast<UIntX*>(ptr);

    *ptrX = src;

    offset += sizeof(UIntX);
}

void serializeRaw(std::vector<uint8_t>& dst, uint32_t& offset, std::span<const uint8_t> src) {
    const size_t size = offset + src.size();
    if (dst.size() < size) {
        dst.resize(size);
    }

    uint8_t* ptr = dst.data() + offset;
    memcpy(ptr, src.data(), src.size());

    offset += roundSize(src.size());
}

template<typename UIntX>
void deserializeUIntX(std::span<const uint8_t> src, uint32_t& offset, UIntX& dst) {
    assert(src.size() >= offset + sizeof(UIntX));

    const uint8_t* ptr = src.data() + offset;
    const UIntX* ptrX = reinterpret_cast<const UIntX*>(ptr);

    dst = *ptrX;

    offset += sizeof(UIntX);
}

void deserializeRaw(std::span<const uint8_t> src, uint32_t& offset, std::span<uint8_t> dst) {
    assert(src.size() >= offset + dst.size());

    const uint8_t* ptr = src.data() + offset;
    memcpy(dst.data(), ptr, dst.size());

    offset += roundSize(dst.size());
}

BinCacheFile deserializeRawData(std::span<const uint8_t> src) {
    uint32_t offset = 0;
    BinCacheFile result{};

    deserializeUIntX(src, offset, result.version);
    assert(result.version == VERSION);

    uint32_t elementsCount = 0;
    deserializeUIntX(src, offset, elementsCount);
    result.elements.resize(elementsCount);

    for (auto& element : result.elements) {
        deserializeUIntX(src, offset, element.hashType);

        switch (element.hashType) {
            case CacheHashType::FullSource: {
                uint32_t keySize = 0;
                deserializeUIntX(src, offset, keySize);

                std::string key(keySize, 0);
                deserializeRaw(src, offset, {reinterpret_cast<uint8_t*>(key.data()), key.size()});

                snap::rhi::backend::opengl::hash64 keyHash = snap::rhi::backend::opengl::build_hash64(key);
                element.key = keyHash;
            } break;

            case CacheHashType::Hash64: {
                snap::rhi::backend::opengl::hash64 keyHash = 0;
                deserializeUIntX(src, offset, keyHash);

                element.key = keyHash;
            } break;

            default: {
                snap::rhi::common::throwException("[getSerializedDataSize] invalid hashType");
            }
        }

        uint32_t format = 0;
        deserializeUIntX(src, offset, format);
        element.format = static_cast<GLenum>(format);

        uint32_t dataSize = 0;
        deserializeUIntX(src, offset, dataSize);
        element.binData.resize(dataSize, 0);
        deserializeRaw(src, offset, {reinterpret_cast<uint8_t*>(element.binData.data()), element.binData.size()});
    }

    return result;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
PipelineCacheStorage deserialize(std::span<const uint8_t> src) {
    PipelineCacheStorage result{};

    if (src.empty()) {
        return result;
    }

    BinCacheFile data = deserializeRawData(src);
    for (auto& element : data.elements) {
        [[maybe_unused]] const auto& [itr, inserted] = result.try_emplace(
            std::get<snap::rhi::backend::opengl::hash64>(element.key), element.format, std::move(element.binData));
        assert(inserted);
    }

    return result;
}

bool serialize(const PipelineCacheStorage& src, std::vector<uint8_t>& dst) {
    uint32_t offset = 0;

    serializeUIntX(dst, offset, VERSION);                           // version
    serializeUIntX(dst, offset, static_cast<uint32_t>(src.size())); // cache elements amount

    for (const auto& [key, value] : src) {
        const auto& [format, data] = value;
        serializeUIntX(dst, offset, static_cast<uint32_t>(CacheHashType::Hash64)); // hash type
        serializeUIntX(dst, offset, static_cast<uint64_t>(key));                   // hash64 value
        serializeUIntX(dst, offset, static_cast<uint32_t>(format));                // format
        serializeUIntX(dst, offset, static_cast<uint32_t>(data.size()));           // data size
        serializeRaw(dst, offset, {data.data(), data.size()});                     // data value
    }

    return true;
}
} // namespace snap::rhi::backend::opengl
