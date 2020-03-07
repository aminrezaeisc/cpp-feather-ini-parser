/*
   Feather INI Parser - 2.0
   You are free to use this however you wish.

   If you find a bug, please attempt to debug the cause.
   Post your environment details and the cause or fix in the issues section of GitHub.

   Written by Turbine1991.

   Website:
   http://code.google.com/p/feather-ini-parser/downloads

   Help:
   Bundled example & readme.
   https://github.com/Turbine1991/cpp-feather-ini-parser/tree/master/example
*/

#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <utility>
#include <vector>

#ifndef FINI_WIDE_SUPPORT
typedef std::stringstream fini_sstream_t;
typedef std::string fini_string_t;
typedef char fini_char_t;
typedef std::ifstream fini_ifstream_t;
typedef std::ofstream fini_ofstream_t;
#else
typedef std::wstringstream fini_sstream_t;
typedef std::wstring fini_string_t;
typedef wchar_t fini_char_t;
typedef std::wifstream fini_ifstream_t;
typedef std::wofstream fini_ofstream_t;
#endif


fini_string_t &l_trim(fini_string_t &str, const fini_string_t &trim_chars = "\t\v\f; ") {
    str.erase(0, str.find_first_not_of(trim_chars));
    return str;
}

fini_string_t &r_trim(fini_string_t &str, const fini_string_t &trim_chars = "\t\v\f; ") {
    str.erase(str.find_last_not_of(trim_chars) + 1);
    return str;
}

fini_string_t &trim(fini_string_t &str, const fini_string_t &trim_chars = "\t\v\f; ") {
    return l_trim(r_trim(str, trim_chars), trim_chars);
}

template<typename T>
T convert_to(const fini_string_t &str) {
    fini_sstream_t ss(str);
    T num;
    ss >> num;
    return num;
}

template<>
fini_string_t convert_to<fini_string_t>(const fini_string_t &str) {
    return str;
}

template<>
const char *convert_to<const char *>(const fini_string_t &str) {
    return str.c_str();
}

///
class INI {
public:
/// Define
    static int PARSE_FLAGS, SAVE_FLAGS;

    typedef fini_char_t data_t;

    typedef typename std::map<fini_string_t, fini_string_t> keys_t;
    typedef typename std::map<fini_string_t, keys_t *> sections_t;
    typedef typename keys_t::value_type keys_pair_t;
    typedef typename sections_t::value_type sections_pair_t;

    enum source_e {
        SOURCE_FILE, SOURCE_MEMORY
    };
    enum save_e {
        SAVE_PRUNE = 1 /*1 << 0*/,
        SAVE_PADDING_SECTIONS = 2 /*1 << 1*/,
        SAVE_SPACE_SECTIONS = 4 /*1 << 2*/,
        SAVE_SPACE_KEYS = 8 /*1 << 3*/,
        SAVE_TAB_KEYS = 16 /*1 << 4*/,
        SAVE_SEMICOLON_KEYS = 32 /*1 << 5*/
    };
    enum parse_e {
        // PARSE_COMMENTS_SLASH = 1 << 0, PARSE_COMMENTS_HASH = 1 << 1, PARSE_COMMENTS_ALL = 1 << 2
        PARSE_COMMENTS_SLASH = 1, PARSE_COMMENTS_HASH = 2, PARSE_COMMENTS_ALL = 4
    };

/// Data
    const source_e source;
    const fini_string_t filename;

    keys_t *current{};
    sections_t sections;

/// Methods
    INI(const INI &from);

    INI(fini_string_t filename, bool doParse, int parseFlags = 0);

    ~INI();

    void clear();

    bool parse(int parseFlags = 0);

    void _parseFile(fini_ifstream_t &file, unsigned int parseFlags);

    bool save(const fini_string_t &file_name, unsigned int saveFlags = 0);

    keys_t &operator[](const fini_string_t &section);

    void create(const fini_string_t &section);

    void remove(const fini_string_t &section);

    bool select(const fini_string_t &section, bool noCreate = false);

    fini_string_t get(const fini_string_t &section, const fini_string_t &key, fini_string_t def);

    fini_string_t get(const fini_string_t &key, fini_string_t def);

    template<class T>
    T getAs(const fini_string_t &section, fini_string_t key, T def = T());

    template<class T>
    T getAs(const fini_string_t &key, T def = T());

    void set(const fini_string_t &section, const fini_string_t &key, fini_string_t value);

    void set(const fini_string_t &key, fini_string_t value);
};

/// Definition
INI::INI(const INI &from) : source(from.source), filename(from.filename) {
    // Deep clone INI
    for (const auto &i: from.sections) {
        select(i.first);
        for (const auto &j: *i.second)
            set(j.first, j.second);
    }
}

INI::INI(fini_string_t filename, bool doParse, int parseFlags) : source(SOURCE_FILE), filename(std::move(filename)) {
    this->create("");

    if (doParse)
        parse(parseFlags);
}

INI::~INI() {
    clear();
}

void INI::clear() {
    sections.clear();
}

bool INI::parse(int parseFlags) {
    parseFlags = (parseFlags > 0) ? parseFlags : PARSE_FLAGS;

    switch (source) {
        case SOURCE_FILE: {
            fini_ifstream_t file(filename);

            if (!file.is_open())
                return false;

            _parseFile(file, parseFlags);

            file.close();
        }
            break;

        case SOURCE_MEMORY:
            /*std::stringstream sstream;
            sstream.rdbuf()->pubsetbuf(data, dataSize);
            parse(sstream);*/
            break;
    }

    return true;
}

int INI::PARSE_FLAGS = 0, INI::SAVE_FLAGS = 0;

void INI::_parseFile(fini_ifstream_t &file, unsigned int parseFlags) {
    fini_string_t line;
    fini_string_t section; // Set default section (support for section_less files)
    size_t i = 0;
    keys_t *local_current = this->current;

    while (std::getline(file, line)) {
        i++;

        // Parse comments
        if (parseFlags & PARSE_COMMENTS_SLASH || parseFlags & PARSE_COMMENTS_ALL)
            line = line.substr(0, line.find("//"));
        if (parseFlags & PARSE_COMMENTS_HASH || parseFlags & PARSE_COMMENTS_ALL)
            line = line.substr(0, line.find('#'));

        if (trim(line).empty()) // Skip empty lines
            continue;

        if ('[' == line.at(0)) { // Section
            section = trim(line, "[] "); // Obtain key value, including contained spaced
            if (section.empty()) // If no section value, use default section
                local_current = this->current;

            if (sections.find(section) != sections.end()) {
                std::cerr << "Error: cpp-feather-ini-parser: Duplicate section '" + section + "':" << i << std::endl;
                throw std::exception();
            }

            local_current = new keys_t;
            sections[section] = local_current;
        } else {
            size_t indexEquals = line.find('=');
            if (indexEquals != fini_string_t::npos) {
                fini_string_t key = line.substr(0, indexEquals), value = line.substr(indexEquals + 1);
                r_trim(key);
                l_trim(value);

                if ((*local_current).find(key) != (*local_current).end()) {
                    std::cerr << "Error: cpp-feather-ini-parser: Duplicate key '" + key + "':" << i << std::endl;
                    throw std::exception();
                }

                (*local_current).emplace(key, value);
            }
        }
    }
}

bool INI::save(const fini_string_t &file_name, unsigned int saveFlags) {
    saveFlags = (saveFlags > 0) ? saveFlags : SAVE_FLAGS;

    fini_ofstream_t file((file_name.empty()) ? this->filename : file_name, std::ios::trunc);
    if (!file.is_open())
        return false;

    // Save remaining sections
    for (const auto &i: sections) {
        //if (i.first == "")
        //  continue;
        if (saveFlags & SAVE_PRUNE && i.second->empty())  // No keys/values in section, skip to next
            continue;

        // Write section
        if (!i.first.empty()) {
            if (saveFlags & SAVE_SPACE_SECTIONS)
                file << "[ " << i.first << " ]" << std::endl;
            else
                file << '[' << i.first << ']' << std::endl;
        }

        // Loop through key & values
        for (const auto &j: *i.second) {
            if (saveFlags & SAVE_PRUNE && j.second.empty())
                continue;

            // Write key & value
            if (saveFlags & SAVE_TAB_KEYS && !i.first.empty())
                file << '\t'; // Insert indent

            if (saveFlags & SAVE_SPACE_KEYS)
                file << j.first << " = " << j.second;
            else
                file << j.first << '=' << j.second;

            if (saveFlags & SAVE_SEMICOLON_KEYS)
                file << ';';

            file << std::endl;
        }

        // New section line
        if (saveFlags & SAVE_PADDING_SECTIONS)
            file << '\n';
    }

    file.close();

    return true;
}

//Provide bracket access to section contents
INI::keys_t &INI::operator[](const fini_string_t &section) {
    select(section);
    return *current;
}

//Create a new section and select it
void INI::create(const fini_string_t &section) {
    if (!section.empty() && sections.find(section) != sections.end()) {
        std::cerr << "Error: cpp-feather-ini-parser: Duplicate section '" << section << "'" << std::endl;
        throw std::exception();
    }

    current = new keys_t;
    sections[section] = current;
}

//Removes a section including all key/value pairs
void INI::remove(const fini_string_t &section) {
    if (select(section, true))
        sections.erase(section);

    current = nullptr;
}

//Select a section for performing operations
bool INI::select(const fini_string_t &section, bool noCreate) {
    // auto -> sections_t::iterator
    auto section_it = sections.find(section);
    if (section_it == sections.end()) {
        if (!noCreate) { create(section); }
        return false;
    }
    current = section_it->second;
    return true;
}

fini_string_t INI::get(const fini_string_t &section, const fini_string_t &key, fini_string_t def) {
    select(section);
    return get(key, std::move(def));
}

fini_string_t INI::get(const fini_string_t &key, fini_string_t def) {
    auto it = current->find(key);
    if (it == current->end()) { return def; }
    return it->second;
}

template<class T>
T INI::getAs(const fini_string_t &section, fini_string_t key, T def) {
    select(section);
    return getAs<T>(key, def);
}

template<class T>
T INI::getAs(const fini_string_t &key, T def) {
    auto it = current->find(key);
    if (it == current->end()) { return def; }
    return convert_to<T>(it->second);
}

void INI::set(const fini_string_t &section, const fini_string_t &key, fini_string_t value) {
    if (!select(section)) { create(section); }
    set(key, std::move(value));
}

void INI::set(const fini_string_t &key, fini_string_t value) {
    (*current)[key] = std::move(value);
}
