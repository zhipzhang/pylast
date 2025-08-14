#ifndef TTREE_SERIALIZER_H
#define TTREE_SERIALIZER_H

#include <TTree.h>
#include <string_view>
#include <utility>

// Include the header-only Boost.PFR library
// You'll need to have Boost installed and in your include path.
#include <pfr.hpp>

/**
 * @brief A generic serializer for plain C++ structs to ROOT TTree branches.
 */
class TTreeSerializer {
private:
    // Implementation detail: uses a helper function with an index sequence
    template <typename T, std::size_t... Is>
    static void branch_impl(TTree* tree, T& data_struct, std::index_sequence<Is...>, std::string prefix_name = "") {
        (
            [&]() {
                using FieldType = std::remove_reference_t<decltype(pfr::get<Is>(data_struct))>;
                const char* branch_name = pfr::get_name<Is, T>().data();
                std::string branch_name_str{branch_name};
                if (!prefix_name.empty()) {
                    branch_name_str = (prefix_name + "_" + branch_name);
                }
                if constexpr (std::is_fundamental_v<FieldType>) {
                    tree->Branch(branch_name_str.c_str(), &pfr::get<Is>(data_struct));
                } 
            }(), ...
        );
    }

    template <typename T, std::size_t... Is>
    static void set_branch_addresses_impl(TTree* tree, T& data_struct, std::index_sequence<Is...>, std::string prefix_name="") {
        (
            [&]() {
                using FieldType = std::remove_reference_t<decltype(pfr::get<Is>(data_struct))>;
                const char* branch_name = pfr::get_name<Is, T>().data();
                std::string branch_name_str{branch_name};
                if (!prefix_name.empty()) {
                    branch_name_str = (prefix_name + "_" + branch_name);
                }
                if constexpr (std::is_fundamental_v<FieldType>) {
                    tree->SetBranchAddress(branch_name_str.c_str(), &pfr::get<Is>(data_struct));
                }
            }(), ...
        );
    }

public:
    /**
     * @brief Creates branches in a TTree for each member of a struct.
     * @tparam T The type of the struct. Must be a plain aggregate.
     * @param tree The TTree to add branches to.
     * @param data_struct An instance of the struct to get member addresses from.
     */
    template <typename T>
    static void branch(TTree* tree, T& data_struct, std::string prefix_name ="") {
        branch_impl(tree, data_struct, std::make_index_sequence<pfr::tuple_size_v<T>>{}, prefix_name);
    }

    /**
     * @brief Sets branch addresses in a TTree for each member of a struct.
     * @tparam T The type of the struct. Must be a plain aggregate.
     * @param tree The TTree to read branches from.
     * @param data_struct An instance of the struct to store the read data.
     */
    template <typename T>
    static void set_branch_addresses(TTree* tree, T& data_struct, std::string prefix_name="") {
        set_branch_addresses_impl(tree, data_struct, std::make_index_sequence<pfr::tuple_size_v<T>>{}, prefix_name);
    }
};

#endif // TTREE_SERIALIZER_H
// TTreeReader.h (根据您的要求修改后的版本)
#ifndef TTREE_READER_H
#define TTREE_READER_H

#include <TTree.h>
#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <stdexcept>
#include <type_traits> // 包含头文件
#include <pfr.hpp>

// --- 用于创建“混合存储”元组的元编程助手 ---
template <typename T, typename Indices = std::make_index_sequence<pfr::tuple_size_v<T>>>
struct make_hybrid_storage;

template <typename T, std::size_t... Is>
struct make_hybrid_storage<T, std::index_sequence<Is...>> {
private:
    template <std::size_t I>
    using field_type = std::remove_reference_t<decltype(pfr::get<I>(std::declval<T&>()))>;
public:
    using type = std::tuple<
        std::conditional_t<
            std::is_fundamental_v<field_type<Is>>,
            field_type<Is>,
            std::add_pointer_t<field_type<Is>>
        >...
    >;
};
template <typename T>
using make_hybrid_storage_t = typename make_hybrid_storage<T>::type;
// --- 助手结束 ---


template <typename T>
class TTreeReader {
private:
    TTree* m_tree;
    long long m_current_entry = -1;

    // 使用新的助手创建混合存储
    make_hybrid_storage_t<T> m_hybrid_storage;

    template<std::size_t... Is>
    void setup_branches(std::index_sequence<Is...>) {
        // 使用 C++17 的折叠表达式和逗号运算符来遍历所有成员
        (void)std::initializer_list<int>{
            (
                [this] {
                    // 获取当前字段的真实类型
                    using FieldType = std::remove_reference_t<decltype(pfr::get<Is>(std::declval<T&>()))>;
                    const char* branch_name = pfr::get_name<Is, T>().data();

                    // 关键：根据类型选择不同的 SetBranchAddress 模式
                    if constexpr (std::is_fundamental_v<FieldType>) {
                        // 模式1: 对于基础类型，直接传递其在元组中存储位置的地址
                        m_tree->SetBranchAddress(branch_name, &std::get<Is>(m_hybrid_storage));
                    } else {
                        // 模式2: 对于对象类型，传递其在元组中存储的指针的地址
                        auto& storage_ptr = std::get<Is>(m_hybrid_storage);
                        storage_ptr = nullptr; // 初始化为空指针
                        m_tree->SetBranchAddress(branch_name, &storage_ptr);
                    }
                }(),
                0
            )...
        };
    }

    template<std::size_t... Is>
    void fill_struct(T& target_struct, std::index_sequence<Is...>) const {
        (void)std::initializer_list<int>{
            (
                [this, &target_struct] {
                    using FieldType = std::remove_reference_t<decltype(pfr::get<Is>(std::declval<T&>()))>;
                    auto& struct_member = pfr::get<Is>(target_struct);
                    const auto& storage_member = std::get<Is>(m_hybrid_storage);

                    // 关键：根据类型选择不同的赋值方式
                    if constexpr (std::is_fundamental_v<FieldType>) {
                        // 对于基础类型，直接从元组中的值拷贝
                        struct_member = storage_member;
                    } else {
                        // 对于对象类型，解引用元组中的指针再拷贝
                        if (storage_member) { // 确保指针有效
                           struct_member = *storage_member;
                        }
                    }
                }(),
                0
            )...
        };
    }

public:
    explicit TTreeReader(TTree* tree) : m_tree(tree) {
        if (!m_tree) {
            throw std::invalid_argument("TTree pointer cannot be null.");
        }
        setup_branches(std::make_index_sequence<pfr::tuple_size_v<T>>{});
    }

    long long get_entries() const { return m_tree->GetEntries(); }

    void get_entry(long long entry) {
        m_tree->GetEntry(entry);
        m_current_entry = entry;
    }

    T read() {
        if (m_current_entry < 0) {
            if (get_entries() > 0) get_entry(0); else return T{};
        }
        T data_struct;
        fill_struct(data_struct, std::make_index_sequence<pfr::tuple_size_v<T>>{});
        return data_struct;
    }
};
#endif // TTREE_READER_H