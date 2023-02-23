// Copyright (c) 2018-2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
#pragma once

#include "../Defines.h"

#include "../logging/Log.h"
#include "../../omni/String.h"

#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>


namespace carb
{
namespace extras
{

// Forward declarations
class Path;
inline Path operator/(const Path& left, const Path& right);
inline Path operator+(const Path& left, const Path& right);
inline Path operator+(const Path& left, const char* right);
inline Path operator+(const Path& left, const std::string& right);
inline Path operator+(const Path& left, const omni::string& right);
inline Path operator+(const char* left, const Path& right);
inline Path operator+(const std::string& left, const Path& right);
inline Path operator+(const omni::string& left, const Path& right);

// Constants
static constexpr const char* kEmptyString = "";

static constexpr const char* kDotString = ".";
static constexpr size_t kDotStringLength = 1;

static constexpr const char* kDotDotString = "..";
static constexpr size_t kDotDotStringLength = 2;

static constexpr const char* kForwardSlashString = "/";
static constexpr size_t kForwardSlashStringLength = 1;

static constexpr char kDotChar = '.';
static constexpr char kForwardSlashChar = '/';
static constexpr char kColonChar = ':';
static constexpr char kFirstLowercaseLetter = 'a';
static constexpr char kLastLowercaseLetter = 'z';


/**
 * Path objects are used for filesystem path manipulations.
 *
 * All paths are in UTF8 encoding using forward slash as path separator.
 *
 * Note: the class supports implicit casting to a "std::string" and explicit cast to
 * a "const char *" pointer
 */
class Path
{
public:
    //--------------------------------------------------------------------------------------
    // Constructors/destructor and assignment operations

    Path() = default;

    /**
     * Creates a new path from a possible non zero-terminated char array containing utf8 string
     *
     * @param path A pointer to the data
     * @param pathLen the size of the data to be used to create the path object
     */
    Path(const char* path, size_t pathLen)
    {
        if (path && pathLen)
        {
            m_pathString.assign(path, pathLen);
        }

        _sanitizePath();
    }

    /**
     * Creates a new path from a zero-terminated char array containing utf8 string
     *
     * @param path A pointer to the char array
     */
    Path(const char* path)
    {
        if (path)
        {
            m_pathString = path;
        }
        _sanitizePath();
    }

    /**
     * Creates a new path from a utf8 std string
     *
     * @param path The source string
     */
    Path(std::string path) : m_pathString(std::move(path))
    {
        _sanitizePath();
    }

    /**
     * Creates a new path from an omni::string
     *
     * @param path The source string
     */
    Path(const omni::string& path) : m_pathString(path.c_str())
    {
        _sanitizePath();
    }

    Path(const Path& other) : m_pathString(other.m_pathString)
    {
    }

    Path(Path&& other) noexcept : m_pathString(std::move(other.m_pathString))
    {
    }

    Path& operator=(const Path& other)
    {
        m_pathString = other.m_pathString;
        return *this;
    }

    Path& operator=(Path&& other) noexcept
    {
        m_pathString = std::move(other.m_pathString);
        return *this;
    }

    ~Path() = default;

    //--------------------------------------------------------------------------------------
    // Getting a string representation of the internal data

    /**
     * Gets the std string representation of the path
     *
     * @return std string representation
     */
    std::string getString() const
    {
        return m_pathString;
    }

    /**
     * Implicit conversion operator to the std string
     *
     * @return std string representation
     */
    operator std::string() const
    {
        return m_pathString;
    }

    /**
     * Get the const char pointer to the path data
     *
     * @return pointer to the start of the path data
     */
    const char* getStringBuffer() const
    {
        return m_pathString.c_str();
    }

    /**
     * Streaming operator to stream path.
     */
    friend std::ostream& operator<<(std::ostream& os, const Path& path)
    {
        os << path.m_pathString;
        return os;
    }

    /**
     * Explicit conversion operator to the pointer to const char
     *
     * @return pointer to the start of the path data
     */
    explicit operator const char*() const
    {
        return getStringBuffer();
    }

    //--------------------------------------------------------------------------------------
    // Path operations

    // Compare operations
    bool operator==(const Path& other) const
    {
        return m_pathString == other.m_pathString;
    }

    bool operator==(const std::string& other) const
    {
        return m_pathString == other;
    }

    bool operator==(const char* other) const
    {
        if (other == nullptr)
        {
            return false;
        }
        return m_pathString == other;
    }

    bool operator!=(const Path& other) const
    {
        return !(*this == other);
    }

    bool operator!=(const std::string& other) const
    {
        return !(*this == other);
    }

    bool operator!=(const char* other) const
    {
        return !(*this == other);
    }

    /**
     * Gets the length of the path
     *
     *@return length of the path
     */
    size_t getLength() const
    {
        return m_pathString.size();
    }

    /**
     * Clears current path
     *
     * @return Reference to the current object
     */
    Path& clear()
    {
        m_pathString.clear();
        return *this;
    }

    /**
     * Checks if the path is an empty string
     *
     * @return True if the path contains at least one character, false otherwise
     */
    bool isEmpty() const
    {
        return m_pathString.empty();
    }

    /**
     * Returns the filename component of the path, or an empty path object if there is no filename.
     *
     * @return The path object representing the filename
     */
    Path getFilename() const
    {
        const char* filenamePointer = _getFilenamePointer();
        if (!filenamePointer)
        {
            return Path();
        }

        const size_t filenameOffset = filenamePointer - m_pathString.data();
        return Path(m_pathString.substr(filenameOffset, m_pathString.size() - filenameOffset));
    }

    /**
     * Returns the extension of the filename component of the path, including period (.), or an empty path object.
     *
     * @return The path object representing the extension
     */
    Path getExtension() const
    {
        const char* extPointer = _getExtensionPointer();
        if (!extPointer)
        {
            return Path();
        }

        const size_t extOffset = extPointer - m_pathString.data();
        return Path(m_pathString.substr(extOffset, m_pathString.size() - extOffset));
    }

    /**
     * Returns the path to the parent directory, or an empty path object if there is no parent.
     *
     * @return The path object representing the parent directory
     */
    Path getParent() const;

    /**
     * Returns the filename component of the path stripped of the extension, or an empty path object if there is no
     * filename.
     *
     * @return The path object representing the stem
     */
    Path getStem() const
    {
        const char* extPointer = _getExtensionPointer();

        if (extPointer == nullptr)
        {
            return getFilename();
        }

        const char* filenamePointer = _getFilenamePointer();
        return Path(m_pathString.substr(filenamePointer - m_pathString.data(), extPointer - filenamePointer));
    }

    /**
     * Returns the root name in the path
     *
     * @return The path object representing the root name
     */
    Path getRootName() const
    {
        const char* rootNameEndPointer = _getRootNameEndPointer();
        if (!rootNameEndPointer)
        {
            return Path();
        }

        return Path(m_pathString.substr(0, rootNameEndPointer - m_pathString.data()));
    }

    /**
     * Returns the relative part of the path (the part after optional root name and root directory)
     *
     * @return The path objects representing the relative part of the path
     */
    Path getRelativePart() const
    {
        const char* relativePartPointer = _getRelativePartPointer();
        if (relativePartPointer == nullptr)
        {
            return Path();
        }

        const size_t relativePartOffset = relativePartPointer - m_pathString.data();

        return Path(m_pathString.substr(relativePartOffset, m_pathString.size() - relativePartOffset));
    }

    /**
     * Returns the root directory if it's present in the path
     *
     * @return The path object representing the root directory
     */
    Path getRootDirectory() const
    {
        const char* rootDirectoryEndPointer = _getRootDirectoryEndPointer();

        if (!rootDirectoryEndPointer)
        {
            return Path();
        }
        const char* rootNameEndPointer = _getRootNameEndPointer();
        return Path(rootDirectoryEndPointer == rootNameEndPointer ? kEmptyString : kForwardSlashString);
    }

    /**
     * Checks if the path has a root directory
     *
     * @return the result of the check
     */
    bool hasRootDirectory() const noexcept
    {
        return !getRootDirectory().isEmpty();
    }

    /**
     * Returns the root of the path i.e. the root name + root directory if they are present
     *
     * @return The path object representing the root of the path
     */
    Path getRoot() const
    {
        const char* rootDirectoryEndPointer = _getRootDirectoryEndPointer();
        if (!rootDirectoryEndPointer)
        {
            return Path();
        }

        return Path(m_pathString.substr(0, rootDirectoryEndPointer - m_pathString.data()));
    }

    /**
     * A helper function to add together two path without checking for a separator and adding it
     *
     * @ return The path object that has the unified data from the both paths
     */
    Path concat(const Path& concatedPart) const
    {
        if (isEmpty())
        {
            return concatedPart;
        }
        if (concatedPart.isEmpty())
        {
            return *this;
        }

        PathPartDescription parts[] = { { getStringBuffer(), getLength() },
                                        { concatedPart.getStringBuffer(), concatedPart.getLength() } };

        return _concat(parts, CARB_COUNTOF(parts));
    }

    /**
     * A helper function to add together two path with checking for a separator and adding it if needed
     *
     * @ return The path object that has the unified data from the both paths
     */
    Path join(const Path& joinedPart) const;

    Path& operator/=(const Path& path)
    {
        return *this = *this / path;
    }

    Path& operator+=(const Path& path)
    {
        return *this = *this + path;
    }

    /**
     * A helper function to change the extension part of the current path
     *
     * @param newExtension The path containing the data for the new extension
     *
     * @return The changed current object
     */
    Path& replaceExtension(const Path& newExtension);

    /**
     * Makes an absolute path as normalizing the addition of the current path to the root.
     * This function does NOT make absolute path using the CWD as a root. You need to use the carb::filesystem
     * plugin to get the current working directory and pass it to this function for a such purpose.
     *
     * @return The path object representing the constructed absolute path
     */
    Path getAbsolute(const Path& root = "") const
    {
        if (isAbsolute() || root.isEmpty())
        {
            return this->getNormalized();
        }
        return root.join(*this).getNormalized();
    }

    /**
     * Returns the result of the normalization of the current path
     *
     * @return A new path object representing the normalized current path
     */
    Path getNormalized() const;

    /**
     * Normalizes current path in place
     *
     * @return reference to the current object
     */
    Path& normalize()
    {
        return *this = getNormalized();
    }

    /**
     * Checks if the current path is an absolute path
     *
     * @return True if the current path is an absolute path, false otherwise.
     */
    bool isAbsolute() const;

    /**
     * Checks if the current path is the relative path
     *
     * @return True if the current path is a relative path, false otherwise.
     */
    bool isRelative() const
    {
        return !isAbsolute();
    }

    /**
     * Returns current path made relative to base
     * Note: the function does NOT normalize the paths prior to the operation
     *
     * @param base path servicing as a base
     *
     * @return an empty path if it's impossible to match roots (different root names, different states of being
     * relative/absolute with a base path, not having a root directory while the base has it), otherwise a non-empty
     * relative path
     */
    Path getRelative(const Path& base) const noexcept;

private:
    enum class PathTokenType
    {
        Slash,
        RootName,
        Dot,
        DotDot,
        Name
    };

    // Helper function to parse next path token starting at bufferStart till bufferEnd (points after the end of the
    // buffer data). On success returns pointer immediately after the token data and returns token type in the
    // resultType. On failure returns null and the value of the resultType is undetermined.
    // Note: it doesn't determine if a Name is a RootName. (RootName is added to enum for convenience)
    static const char* _getTokenEnd(const char* bufferBegin, const char* bufferEnd, PathTokenType& resultType)
    {
        if (bufferBegin == nullptr || bufferEnd == nullptr || bufferEnd <= bufferBegin)
        {
            return nullptr;
        }

        // Trying to find the next slash
        const char* tokenEnd = _findFromStart(bufferBegin, bufferEnd - bufferBegin, kForwardSlashChar);
        // If found a slash as the first symbol then return pointer to the data after it
        if (tokenEnd == bufferBegin)
        {
            resultType = PathTokenType::Slash;
            return tokenEnd + 1;
        }

        // If no slash found we consider all passed data as a single token
        if (!tokenEnd)
        {
            tokenEnd = bufferEnd;
        }

        const size_t tokenSize = tokenEnd - bufferBegin;
        if (tokenSize == 1 && *bufferBegin == kDotChar)
        {
            resultType = PathTokenType::Dot;
        }
        else if (tokenSize == 2 && bufferBegin[0] == kDotChar && bufferBegin[1] == kDotChar)
        {
            resultType = PathTokenType::DotDot;
        }
        else
        {
            resultType = PathTokenType::Name;
        }
        return tokenEnd;
    }

    struct PathPartDescription
    {
        const char* data;
        size_t size;
    };

    /**
     * Helper fuction to add together several parts of the path into a new one
     */
    static Path _concat(const PathPartDescription* pathParts, size_t numParts)
    {
        if (!pathParts || numParts == 0)
        {
            return Path();
        }

        size_t totalSize = 0;

        for (size_t i = 0; i < numParts; ++i)
        {
            if (pathParts[i].data)
            {
                totalSize += pathParts[i].size;
            }
        }

        if (totalSize == 0)
        {
            return Path();
        }

        std::string buffer;
        buffer.reserve(totalSize);

        for (size_t i = 0; i < numParts; ++i)
        {
            const char* partData = pathParts[i].data;
            const size_t partSize = pathParts[i].size;

            if (partData && partSize > 0)
            {
                buffer.append(partData, partSize);
            }
        }

        return Path(std::move(buffer));
    }

    /**
     * Helper function to perform a backward search of a character in a memory buffer
     */
    template <class Pred = std::equal_to<char>>
    static const char* _findFromEnd(const char* data, size_t dataSize, char ch)
    {
        if (!data || dataSize == 0)
        {
            return nullptr;
        }

        --data;

        Pred pred;
        while (dataSize > 0)
        {
            if (pred(data[dataSize], ch))
            {
                return data + dataSize;
            }
            --dataSize;
        }
        return nullptr;
    }

    /**
     * Helper function to perform a forward search of a character in a memory buffer
     */
    template <class Pred = std::equal_to<char>>
    static const char* _findFromStart(const char* data, size_t dataSize, char ch)
    {
        if (!data || dataSize == 0)
        {
            return nullptr;
        }

        Pred pred;
        for (const char* const dataEnd = data + dataSize; data != dataEnd; ++data)
        {
            if (pred(*data, ch))
            {
                return data;
            }
        }
        return nullptr;
    }

    /**
     * Helper function to find the pointer to the start of the filename
     */
    const char* _getFilenamePointer() const
    {
        if (isEmpty())
        {
            return nullptr;
        }

        const char* pathDataStart = m_pathString.data();
        const size_t pathDataSize = m_pathString.size();

        // Find the last slash
        const char* slashPointer = _findFromEnd(pathDataStart, pathDataSize, kForwardSlashChar);
        // No slash == only filename
        if (!slashPointer)
        {
            return pathDataStart;
        }

        const char* filenamePointer = slashPointer + 1;
        // Check that there is any data after the last slash
        if (filenamePointer >= pathDataStart + pathDataSize)
        {
            return nullptr;
        }

        return filenamePointer;
    }

    /**
     * Helper function to find the pointer to the start of the extension
     */
    const char* _getExtensionPointer() const
    {
        const char* filenamePointer = _getFilenamePointer();
        if (filenamePointer == nullptr)
        {
            return nullptr;
        }

        const char* pathDataStart = m_pathString.data();
        const size_t pathDataSize = m_pathString.size();

        const size_t filenameOffset = filenamePointer - pathDataStart;

        const char* extStart = _findFromEnd(filenamePointer, pathDataSize - filenameOffset, kDotChar);

        // checking if the pointer is at the last position (i.e. filename ends with just a dot(s))
        if (extStart == pathDataStart + pathDataSize - 1)
        {
            extStart = nullptr;
        }

        return extStart != filenamePointer ? extStart : nullptr;
    }

    /**
     * Helper function to find the pointer to the end of the root name (the pointer just after the end of the root name)
     */
    const char* _getRootNameEndPointer() const
    {
        if (isEmpty())
        {
            return nullptr;
        }

        const char* pathDataStart = m_pathString.data();
        const size_t pathDataSize = m_pathString.size();

        if (pathDataSize < 2)
        {
            return pathDataStart;
        }

#if CARB_PLATFORM_WINDOWS
        // Checking if the path starts with a drive letter followed by a colon (ex: "A:/...")
        if (pathDataStart[1] == kColonChar)
        {
            const char firstLetter = static_cast<char>(std::tolower(pathDataStart[0]));
            if (kFirstLowercaseLetter <= firstLetter && firstLetter <= kLastLowercaseLetter)
            {
                return pathDataStart + 2;
            }
        }
#endif
        // Checking if it's a UNC name (ex: "//location/...")
        // Note: Just checking if the first 2 chars are forward slashes and the third symbol is not
        if (pathDataSize >= 3 && pathDataStart[0] == kForwardSlashChar && pathDataStart[1] == kForwardSlashChar &&
            pathDataStart[2] != kForwardSlashChar)
        {
            // searching for a root directory
            const char* const slashPointer = _findFromStart(pathDataStart + 3, pathDataSize - 3, kForwardSlashChar);
            return slashPointer ? slashPointer : pathDataStart + pathDataSize;
        }

        return pathDataStart;
    }

    /**
     * Helper function to get the pointer to the start of the relative part of the path
     */
    const char* _getRelativePartPointer() const
    {
        const char* rootNameEndPointer = _getRootNameEndPointer();

        // The rootNameEndPointer is always not null if the path is not empty
        if (!rootNameEndPointer)
        {
            return nullptr;
        }

        const size_t rootEndOffset = rootNameEndPointer - m_pathString.data();
        // Find the pointer to the first symbol after the root name that is not a forward slash
        return _findFromStart<std::not_equal_to<char>>(
            rootNameEndPointer, m_pathString.size() - rootEndOffset, kForwardSlashChar);
    }

    /**
     * Helper function to find the pointer to the end of the root directory (the pointer just after the end of the root
     * directory)
     */
    const char* _getRootDirectoryEndPointer() const
    {
        const char* rootNameEndPointer = _getRootNameEndPointer();
        const char* relativePartPointer = _getRelativePartPointer();

        if (relativePartPointer != rootNameEndPointer)
        {
            const char* result = rootNameEndPointer + 1;
            if (result > m_pathString.data() + m_pathString.size())
            {
                result = rootNameEndPointer;
            }
            return result;
        }
        return rootNameEndPointer;
    }

    // Patching paths in the constructors (using external string data) if needed
    void _sanitizePath()
    {
#if CARB_PLATFORM_WINDOWS
        constexpr char kBackwardSlashChar = '\\';

        // changing the backward slashes for Windows to forward ones
        for (auto& curChar : m_pathString)
        {
            if (curChar == kBackwardSlashChar)
            {
                curChar = kForwardSlashChar;
            }
        }

#elif CARB_PLATFORM_LINUX
        // Nothing to do here
#endif
    }

    std::string m_pathString;
};

/**
 * A helper operator "+" overload that performs the concat operation
 */
inline Path operator+(const Path& left, const Path& right)
{
    return left.concat(right);
}

inline Path operator+(const Path& left, const char* right)
{
    return left.concat(right);
}

inline Path operator+(const Path& left, const std::string& right)
{
    return left.concat(right);
}

inline Path operator+(const Path& left, const omni::string& right)
{
    return left.concat(right);
}

inline Path operator+(const char* left, const Path& right)
{
    return Path(left).concat(right);
}

inline Path operator+(const std::string& left, const Path& right)
{
    return Path(left).concat(right);
}

inline Path operator+(const omni::string& left, const Path& right)
{
    return Path(left).concat(right);
}

/**
 * A helper operator "/" overload that performs the join operation
 */
inline Path operator/(const Path& left, const Path& right)
{
    return left.join(right);
}

/**
 * Helper function to get a Path object representing parent directory for the provided std string representation of a
 * path
 */
inline Path getPathParent(std::string path)
{
    return Path(std::move(path)).getParent();
}

/**
 * Helper function to get a Path object representing the extension part of the provided std string representation of a
 * path
 */
inline Path getPathExtension(std::string path)
{
    return Path(std::move(path)).getExtension();
}

/**
 * Helper function to get a Path object representing the stem part of the provided std string representation of a
 * path
 */
inline Path getPathStem(std::string path)
{
    return Path(std::move(path)).getStem();
}

/**
 * Helper function to calculate a relative path from a provided path and a base path
 */
inline Path getPathRelative(std::string path, std::string base)
{
    return Path(std::move(path)).getRelative(std::move(base));
}

// Additional compare operators
inline bool operator==(const std::string& left, const Path& right)
{
    return right == left;
}

inline bool operator==(const char* left, const Path& right)
{
    return right == left;
}

inline bool operator!=(const std::string& left, const Path& right)
{
    return right != left;
}

inline bool operator!=(const char* left, const Path& right)
{
    return right != left;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Implementations of large public functions
////////////////////////////////////////////////////////////////////////////////////////////

inline Path Path::getParent() const
{
    const char* parentPathEndPointer = _getFilenamePointer();

    const char* pathDataStart = m_pathString.data();

    if (parentPathEndPointer == nullptr)
    {
        parentPathEndPointer = pathDataStart + m_pathString.size();
    }

    const char* slashesDataStart = pathDataStart;

    if (hasRootDirectory())
    {
        slashesDataStart += (_getRootDirectoryEndPointer() - pathDataStart);
    }

    // Cleaning up the trailing slashes;
    while (parentPathEndPointer > slashesDataStart && parentPathEndPointer[-1] == kForwardSlashChar)
    {
        --parentPathEndPointer;
    }

    if (parentPathEndPointer == pathDataStart)
    {
        return Path();
    }

    return Path(m_pathString.substr(0, parentPathEndPointer - pathDataStart));
}

inline Path Path::join(const Path& joinedPart) const
{
    if (isEmpty())
    {
        return joinedPart;
    }
    if (joinedPart.isEmpty())
    {
        return *this;
    }

    const bool haveSeparator =
        m_pathString.back() == kForwardSlashChar || joinedPart.m_pathString.front() == kForwardSlashChar;

    PathPartDescription parts[3] = {};
    size_t numParts = 0;

    parts[numParts++] = { getStringBuffer(), getLength() };

    if (!haveSeparator)
    {
        parts[numParts++] = { kForwardSlashString, kForwardSlashStringLength };
    }
    parts[numParts++] = { joinedPart.getStringBuffer(), joinedPart.getLength() };

    return _concat(parts, numParts);
}

inline Path& Path::replaceExtension(const Path& newExtension)
{
    const char* extPointer = _getExtensionPointer();

    // Check if we need to just remove the extension
    if (newExtension.isEmpty())
    {
        if (extPointer)
        {
            m_pathString = m_pathString.substr(0, extPointer - m_pathString.data());
        }
        return *this;
    }

    const char* newExtData = newExtension.getStringBuffer();
    size_t newExtSize = newExtension.getLength();

    if (*newExtData == kDotChar)
    {
        ++newExtData;
        --newExtSize;
    }

    size_t remainingPathSize = getLength();

    if (extPointer)
    {
        remainingPathSize = extPointer - m_pathString.data();

        size_t oldExtSize = getLength() - (extPointer - m_pathString.data());
        // skipping starting dot
        --oldExtSize;

        // Checking for trying to use the same extension
        if (oldExtSize == newExtSize && ::memcmp(extPointer + 1, newExtData, newExtSize) == 0)
        {
            return *this;
        }
    }

    PathPartDescription parts[] = { { this->getStringBuffer(), remainingPathSize },
                                    { kDotString, kDotStringLength },
                                    { newExtData, newExtSize } };

    return *this = _concat(parts, CARB_COUNTOF(parts));
}

inline Path Path::getNormalized() const
{
    if (isEmpty())
    {
        return Path();
    }

    constexpr size_t kDefaultTokenCount = 128;

    enum class NormalizePartType
    {
        Slash,
        RootName,
        RootSlash,
        Dot,
        DotDot,
        Name,
        Error
    };

    struct ParsedPathPartDescription : PathPartDescription
    {
        NormalizePartType type;

        ParsedPathPartDescription(const char* partData, size_t partSize, PathTokenType partType)
            : PathPartDescription{ partData, partSize }
        {
            switch (partType)
            {
                case PathTokenType::Slash:
                    type = NormalizePartType::Slash;
                    break;
                case PathTokenType::RootName:
                    type = NormalizePartType::RootName;
                    break;
                case PathTokenType::Dot:
                    type = NormalizePartType::Dot;
                    break;
                case PathTokenType::DotDot:
                    type = NormalizePartType::DotDot;
                    break;
                case PathTokenType::Name:
                    type = NormalizePartType::Name;
                    break;

                default:
                    type = NormalizePartType::Error;
                    CARB_LOG_ERROR("Invalid internal token state while normalizing a path");
                    CARB_ASSERT(false);
                    break;
            }
        }

        ParsedPathPartDescription(const char* partData, size_t partSize, NormalizePartType partType)
            : PathPartDescription{ partData, partSize }, type(partType)
        {
        }
    };

    std::vector<ParsedPathPartDescription> resultPathTokens;

    resultPathTokens.reserve(kDefaultTokenCount);

    const char* prevTokenEnd = _getRootDirectoryEndPointer();
    const char* pathDataStart = m_pathString.data();
    const size_t pathDataLength = getLength();
    if (prevTokenEnd && prevTokenEnd > pathDataStart)
    {
        // Adding the root name and the root directory as different elements
        const char* possibleSlashPos = prevTokenEnd - 1;
        if (*possibleSlashPos == kForwardSlashChar)
        {
            if (possibleSlashPos > pathDataStart)
            {
                resultPathTokens.emplace_back(
                    pathDataStart, static_cast<size_t>(possibleSlashPos - pathDataStart), PathTokenType::RootName);
            }
            resultPathTokens.emplace_back(kForwardSlashString, kForwardSlashStringLength, NormalizePartType::RootSlash);
        }
        else
        {
            resultPathTokens.emplace_back(
                pathDataStart, static_cast<size_t>(prevTokenEnd - pathDataStart), PathTokenType::RootName);
        }
    }
    else
    {
        prevTokenEnd = pathDataStart;
    }

    bool alreadyNormalized = true;
    const char* bufferEnd = pathDataStart + pathDataLength;
    PathTokenType curTokenType = PathTokenType::Name;
    for (const char* curTokenEnd = _getTokenEnd(prevTokenEnd, bufferEnd, curTokenType); curTokenEnd != nullptr;
         prevTokenEnd = curTokenEnd, curTokenEnd = _getTokenEnd(prevTokenEnd, bufferEnd, curTokenType))
    {
        switch (curTokenType)
        {
            case PathTokenType::Slash:
                if (resultPathTokens.empty() || resultPathTokens.back().type == NormalizePartType::Slash ||
                    resultPathTokens.back().type == NormalizePartType::RootSlash)
                {
                    // Skip if we already have a slash at the end
                    alreadyNormalized = false;
                    continue;
                }
                break;

            case PathTokenType::Dot:
                // Just skip it
                alreadyNormalized = false;
                continue;

            case PathTokenType::DotDot:
                if (resultPathTokens.empty())
                {
                    break;
                }
                // Check if the previous element is a part of the root name (even without a slash) and skip dot-dot in
                // such case
                if (resultPathTokens.back().type == NormalizePartType::RootName ||
                    resultPathTokens.back().type == NormalizePartType::RootSlash)
                {
                    alreadyNormalized = false;
                    continue;
                }

                if (resultPathTokens.size() > 1)
                {
                    CARB_ASSERT(resultPathTokens.back().type == NormalizePartType::Slash);

                    const NormalizePartType tokenTypeBeforeSlash = resultPathTokens[resultPathTokens.size() - 2].type;

                    // Remove <name>/<dot-dot> pattern
                    if (tokenTypeBeforeSlash == NormalizePartType::Name)
                    {
                        resultPathTokens.pop_back(); // remove the last slash
                        resultPathTokens.pop_back(); // remove the last named token
                        alreadyNormalized = false;
                        continue; // and we skip the addition of the dot-dot
                    }
                }

                break;

            case PathTokenType::Name:
                // No special processing needed
                break;

            default:
                CARB_LOG_ERROR("Invalid internal state while normalizing the path {%s}", getStringBuffer());
                CARB_ASSERT(false);
                alreadyNormalized = false;
                continue;
        }

        resultPathTokens.emplace_back(prevTokenEnd, static_cast<size_t>(curTokenEnd - prevTokenEnd), curTokenType);
    }

    if (resultPathTokens.empty())
    {
        return Path(kDotString);
    }
    else if (resultPathTokens.back().type == NormalizePartType::Slash && resultPathTokens.size() > 1)
    {
        // Removing the trailing slash for special cases like "./" and "../"
        const size_t indexOfTokenBeforeSlash = resultPathTokens.size() - 2;
        const NormalizePartType typeOfTokenBeforeSlash = resultPathTokens[indexOfTokenBeforeSlash].type;

        if (typeOfTokenBeforeSlash == NormalizePartType::Dot || typeOfTokenBeforeSlash == NormalizePartType::DotDot)
        {
            resultPathTokens.pop_back();
            alreadyNormalized = false;
        }
    }

    if (alreadyNormalized)
    {
        return *this;
    }

    std::vector<PathPartDescription> partsToJoin;
    partsToJoin.reserve(resultPathTokens.size());

    for (const auto& curTokenInfo : resultPathTokens)
    {
        partsToJoin.emplace_back(PathPartDescription{ curTokenInfo.data, curTokenInfo.size });
    }
    return _concat(partsToJoin.data(), partsToJoin.size());
}

inline bool Path::isAbsolute() const
{
#if CARB_POSIX
    return !isEmpty() && m_pathString[0] == kForwardSlashChar;
#elif CARB_PLATFORM_WINDOWS
    // Drive root (D:/abc) case. This is the only position where : is allowed on windows. Checking for separator is
    // important, because D:temp.txt is a relative path on windows.
    const char* pathDataStart = m_pathString.data();
    const size_t pathDataLength = getLength();
    if (pathDataLength > 2 && pathDataStart[1] == kColonChar && pathDataStart[2] == kForwardSlashChar)
        return true;
    // Drive letter (D:) case
    if (pathDataLength == 2 && pathDataStart[1] == kColonChar)
        return true;

    // extended drive letter path (ie: prefixed with "//./D:").
    if (pathDataLength > 4 && pathDataStart[0] == kForwardSlashChar && pathDataStart[1] == kForwardSlashChar &&
        pathDataStart[2] == kDotChar && pathDataStart[3] == kForwardSlashChar)
    {
        // at least a drive name was specified.
        if (pathDataLength > 6 && pathDataStart[5] == kColonChar)
        {
            // a drive plus an absolute path was specified (ie: "//./d:/abc") => succeed.
            if (pathDataStart[6] == kForwardSlashChar)
                return true;

            // a drive and relative path was specified (ie: "//./d:temp.txt") => fail.  We need to
            //   specifically fail here because this path would also get picked up by the generic
            //   special path check below and report success erroneously.
            else
                return false;
        }

        // requesting the full drive volume (ie: "//./d:") => report absolute to match behaviour
        //   in the "d:" case above.
        if (pathDataLength == 6 && pathDataStart[5] == kColonChar)
            return true;
    }

    // check for special paths.  This includes all windows paths that begin with "\\" (converted
    // to unix path separators for our purposes).  This class of paths includes extended path
    // names (ie: prefixed with "\\?\"), device names (ie: prefixed with "\\.\"), physical drive
    // paths (ie: prefixed with "\\.\PhysicalDrive<n>"), removeable media access (ie: "\\.\X:")
    // COM ports (ie: "\\.\COM*"), and UNC paths (ie: prefixed with "\\servername\sharename\").
    //
    // Note that it is not necessarily sufficient to get absolute vs relative based solely on
    // the "//" prefix here without needing to dig further into the specific name used and what
    // it actually represents.  For now, we'll just assume that device, drive, volume, and
    // port names will not be used here and treat it as a UNC path.  Since all extended paths
    // and UNC paths must always be absolute, this should hold up at least for those.  If a
    // path for a drive, volume, or device is actually passed in here, it will still be treated
    // as though it were an absolute path.  The results of using such a path further may be
    // undefined however.
    if (pathDataLength > 2 && pathDataStart[0] == kForwardSlashChar && pathDataStart[1] == kForwardSlashChar &&
        pathDataStart[2] != kForwardSlashChar)
        return true;
    return false;
#else
    CARB_UNSUPPORTED_PLATFORM();
#endif
}

inline Path Path::getRelative(const Path& base) const noexcept
{
    // checking if the operation is possible
    if (getRootName() != base.getRootName() || isAbsolute() != base.isAbsolute() ||
        (!hasRootDirectory() && base.hasRootDirectory()))
    {
        return Path();
    }

    PathTokenType curPathTokenType = PathTokenType::RootName;
    const char* curPathTokenEnd = _getRootDirectoryEndPointer();
    const char* curPathTokenStart = curPathTokenEnd;
    const char* curPathEnd = m_pathString.data() + m_pathString.length();

    PathTokenType basePathTokenType = PathTokenType::RootName;
    const char* basePathTokenEnd = base._getRootDirectoryEndPointer();
    const char* basePathEnd = base.m_pathString.data() + base.m_pathString.length();

    // finding the first mismatch
    for (;;)
    {
        curPathTokenStart = curPathTokenEnd;
        curPathTokenEnd = _getTokenEnd(curPathTokenEnd, curPathEnd, curPathTokenType);

        const char* baseTokenStart = basePathTokenEnd;
        basePathTokenEnd = _getTokenEnd(basePathTokenEnd, basePathEnd, basePathTokenType);

        if (!curPathTokenEnd || !basePathTokenEnd)
        {
            // Checking if both are null
            if (curPathTokenEnd == basePathTokenEnd)
            {
                return Path(kDotString);
            }
            break;
        }

        if (curPathTokenType != basePathTokenType ||
            !std::equal(curPathTokenStart, curPathTokenEnd, baseTokenStart, basePathTokenEnd))
        {
            break;
        }
    }
    int requiredDotDotCount = 0;
    while (basePathTokenEnd)
    {
        if (basePathTokenType == PathTokenType::DotDot)
        {
            --requiredDotDotCount;
        }
        else if (basePathTokenType == PathTokenType::Name)
        {
            ++requiredDotDotCount;
        }

        basePathTokenEnd = _getTokenEnd(basePathTokenEnd, basePathEnd, basePathTokenType);
    }

    if (requiredDotDotCount < 0)
    {
        return Path();
    }

    if (requiredDotDotCount == 0 && !curPathTokenEnd)
    {
        return Path(kDotString);
    }

    const size_t leftoverCurPathSymbols = curPathTokenEnd != nullptr ? curPathEnd - curPathTokenStart : 0;
    const size_t requiredResultSize =
        (kForwardSlashStringLength + kDotDotStringLength) * requiredDotDotCount + leftoverCurPathSymbols;

    Path result;
    result.m_pathString.reserve(requiredResultSize);

    if (requiredDotDotCount > 0)
    {
        result.m_pathString += kDotDotString;
        --requiredDotDotCount;

        for (int i = 0; i < requiredDotDotCount; ++i)
        {
            result.m_pathString += kForwardSlashChar;
            result.m_pathString += kDotDotString;
        }
    }

    bool needsSeparator = !result.m_pathString.empty();
    while (curPathTokenEnd)
    {
        if (curPathTokenType != PathTokenType::Slash)
        {
            if (CARB_LIKELY(needsSeparator))
            {
                result.m_pathString += kForwardSlashChar;
            }
            else
            {
                needsSeparator = true;
            }
            result.m_pathString.append(curPathTokenStart, curPathTokenEnd - curPathTokenStart);
        }

        curPathTokenStart = curPathTokenEnd;
        curPathTokenEnd = _getTokenEnd(curPathTokenEnd, curPathEnd, curPathTokenType);
    }

    return result;
}
} // namespace extras
} // namespace carb
