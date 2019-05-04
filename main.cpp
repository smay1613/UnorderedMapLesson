#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <string>

struct Record
{
    std::string name;
    size_t value;
};

struct BusInfo
{
    size_t number;
    size_t fare;
};

std::ostream& operator<<(std::ostream& stream, const BusInfo& info) {
    stream << "N: " << std::to_string(info.number) << " - F:" << std::to_string(info.fare);
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Record& info) {
    stream << info.name << " " << info.value;
    return stream;
}

struct NoCaseHash
{
    int shiftOffset {1};
    size_t operator()(const Record& record) const // must return size_t
    {
        size_t hash {0};
        for (auto character : record.name) {
            hash <<= shiftOffset; // shift after each iteration
            hash ^= static_cast<size_t>(std::toupper(character));
            // XOR is really a nice choose in most cases, it gives good distribution
        }
        return hash;
    }
};

struct NoCaseEqual
{
    bool operator()(const Record& lhs, const Record& rhs) const {
        if (rhs.name.size() != rhs.name.size()) {
            return false;
        }

        for (size_t i = 0; i < rhs.name.size(); ++i) {
            if (std::toupper(lhs.name[i]) != std::toupper(rhs.name[i])) {
                return false;
            }
        }

        return true;
    }
};

template <class K, class H, class E>
void printUnorderedSet(const std::unordered_set<K, H, E>& set)
{
    for (const auto& key : set) {
        std::cout << key << std::endl;
    }
    std::cout << std::endl;
}

template<class K, class V, class C>
void printUnorderedMap(const std::unordered_map<K, V, C>& mapToPrint)
{
    for (const std::pair<K, V>& entry : mapToPrint) {
        std::cout << "{" << entry.first << ", " << entry.second << "}" << " " << std::endl;
    }
    std::cout << std::endl;
}

void investigateConstructor()
{
    std::unordered_set<std::string> data1 {};
    // will create with implementation defined buckets count
    std::cout << "Default size: " << data1.bucket_count() << std::endl;

//    std::hash<std::string> defaultHashForString;
//    std::unordered_set<Record> data_1 {}; // OOPS! No hash function!

    std::unordered_set<Record, NoCaseHash> data2 {};
    // will work, because Record is struct with members that have operator ==

    std::unordered_set<Record, NoCaseHash, NoCaseEqual> data3 { // specify only initializer list
        {"itvdn", 1300},
        {"ITVDN", 1500},
        {"STL", 2500}
        // NoCaseHash, NoCaseEqual will be used
    };

    printUnorderedSet(data3);

    std::unordered_set<Record, NoCaseHash, NoCaseEqual> data4 { // specify buckets count
        200
        // NoCaseHash, NoCaseEqual will be used
    };

    printUnorderedSet(data4);

    auto equalityTest = [](const Record& lhs, const Record& rhs) {
        return lhs.name == rhs.name;
    };
    auto hasher = [](const Record& record) {
        return std::hash<std::string> {}(record.name) ^
               std::hash<size_t>{}(record.value);
    };

    std::unordered_set<Record,
                       std::function <size_t(const Record&)>, // thats why I prefer decltype and auto
                       decltype (equalityTest)> data5 { // also has version with allocator
        {{"Some data", 200}}, // initializer_list
        100, // buckets count
        hasher, // object of hasher
        equalityTest // object of equality test
    };

    printUnorderedSet(data5);
    // With this vary of params it is easy to do a mistake:
//    std::unordered_set<std::string, NoCaseHash> data6 {NoCaseHash {}};
}

void investigateModifiers(std::unordered_map<std::string, BusInfo>& busSchedule)
{
    busSchedule.insert(std::make_pair("08:00", BusInfo{100, 100})); // too verbose
    busSchedule.emplace("08:00", BusInfo {300, 300});
    // insert/emplace with hint is also available

    std::cout << "Map after insertion:" << std::endl;
    printUnorderedMap(busSchedule);

    auto newBus = busSchedule.emplace("08:00", BusInfo {300, 20}); // (iterator, bool)
    std::cout << "Map after double insertion:" << std::endl;
    printUnorderedMap(busSchedule);

    if (!newBus.second) { // if such key was already presented in container
        // we can modify it!
        /* newBus.first->first = "11:20"; */ // as in std::set, we can't modify key.
        newBus.first->second.fare = 20; // (newBus.first) - part of newBus pair (iterator;bool),
        // (->second) - value of pair (key, value)
        // .fare - BusInfo member
    }
}

void investigateBucketInterface(const std::unordered_map<std::string, BusInfo>& data)
{
    std::cout << "Buckets count: " << data.bucket_count() << std::endl;
    std::cout << "Max buckets count: " << data.max_bucket_count() << std::endl;
    size_t bucket = data.bucket("08:15");
    std::cout << "08:15 will be in " << bucket << std::endl;
    std::cout << "08:15 bucket size: " << data.bucket_size(bucket) << std::endl;

    std::cout << std::endl;
}

void investigateHashPolicy(std::unordered_map<std::string, BusInfo>& data)
{
    std::cout << "Current load factor: " << data.load_factor() << std::endl; // size / capacity
    std::cout << "Load factor 2: " << static_cast<double>(data.size()) / data.bucket_count() << std::endl;
    std::cout << "Max load factor: " << data.max_load_factor() << std::endl;
    std::cout << "Data size: " << data.size() << " Capacity: " << data.bucket_count() << std::endl;

    // if load factor gets close to max load factor, data will be resized
    data.max_load_factor(0.7f);

    data.rehash(100); // make data bucket count >= 100
    std::cout << "Capacity after rehash: " << data.bucket_count() << std::endl;

    data.reserve(200);
    // make room for 200 entries, taking the load factor into account
    // the same as data.rehash(std::ceil(n(200) / data.max_load_factor()));
    std::cout << "Capacity after reserve: " << data.bucket_count() << std::endl;

    auto hasher = [](const Record& record) {
        return std::hash<std::string> {}(record.name) ^
               std::hash<size_t>{}(record.value);
    };
    std::unordered_set<Record, decltype (hasher)> testData {10, hasher};
    constexpr size_t expected = 1000000;
    testData.max_load_factor(0.7f);
    testData.reserve(expected);
    std::cout << "After reserve and max lf: " << testData.bucket_count() << std::endl;
}

int main()
{
    investigateConstructor();

    std::unordered_map<std::string, BusInfo> busSchedule {
        {"08:15", {504, 50}},
        {"08:30", {505, 40}},
        {"08:45", BusInfo {104, 50}},
        {"09:40", BusInfo {105, 30}},
        {"11:20", BusInfo {107, 10}},
        {"11:35", BusInfo {108, 20}},
        {"11:50", BusInfo {109, 30}}
    };

    investigateModifiers(busSchedule);
    investigateBucketInterface(busSchedule);
    investigateHashPolicy(busSchedule);
    return 0;
}
