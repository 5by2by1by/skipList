#include <iostream>
#include <string>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <random>

#define STORE_FILE "./store/dumpFile"
std::string delimiter = ":";    // 分隔符

template<typename K, typename V>
class Node {
    public:
        Node(K, V, int);
        ~Node();

        K get_key() const;
        V get_value() const;
        bool set_value(V);

    Node<K, V>** forward;    // 在不同深度指向下一个节点的指针
    int node_level;    // 本节点所在深度

    private:
        K key;
        V value;
};

template<typename K, typename V>
Node<K, V>::Node (const K k, const V v, int level) : key(k), value(v), node_level(level)  {
    forward = new Node<K, V>* [level + 1];
    memset(forward, 0, sizeof(Node<K, V>*) * (level + 1));
}

template<typename K, typename V>
Node<K, V>::~Node (){
    delete[] forward;       //只删除这个数组，不能删除里面的指针，因为那些不是他分配的
}

template<typename K, typename V>
K Node<K, V>::get_key() const {
    return key;
}

template<typename K, typename V>
V Node<K, V>::get_value() const {
    return value;
}

template<typename K, typename V>
bool Node<K, V> ::set_value(const V val){
    if(value = val)
        return true;
    else
        return false;
}

template<typename K, typename V>
class skipList {
    public:
        explicit skipList(int);         // 编译器  不会  将一个显式的类型转换运算符用于隐式类型转换。
        ~skipList();

        int get_random_level();  // 随机获得层数，用来模拟跳表形式
        Node<K, V>* create_node(K, V, int);

        int insert_element(K, V);   // 增删改查
        int delete_element(K);
        int change_element(K, V);
        bool search_element(K);

        void display_list();
        int size();
        void dump_file();
        void load_file();

    private:
        int _max_level;        // 跳表最高层数
        int _skip_list_level;  // 跳表当前层数
        Node<K, V>* _header;   // 链表头节点
        int _element_count;    // 跳表节点总数

        std::default_random_engine e;  // 随机无符号整数的通用源
        std::mutex mtx;
        std::ofstream _file_writer;
        std::ifstream _file_reader;

        // 持久化操作需要的子方法
        void get_key_value_from_string(const std::string& str, std::string& key,
                                        const std::string& value);
        bool is_valid_string(const std::string& str);
};

template<typename K, typename V>
int skipList<K, V> ::size() {
    return _element_count;
}

template<typename K, typename V>
int skipList<K, V>::get_random_level(){
    int n = 0;                            // 此处 n 应该为0，第0层为最底层，各有50%的概率向上
    std::bernoulli_distribution u(0.5);
    while(u(e)){
        n++;
    }
    return n > _max_level ? _max_level : n;
}

template<typename K, typename V>
Node <K, V>* skipList<K, V> ::create_node(const K k, const V v, int level){
    auto* node = new Node<K, V> (k, v, level);
    return node;
}

template<typename K, typename V>
skipList<K, V>::skipList(int max_level) : _max_level(max_level), _skip_list_level(0), _element_count(0) {
    K k;    V v;
    _header = new Node<K, V> (k, v, _max_level);
    e.seed(time(nullptr));         // 以时间为种子
}

template<typename K, typename V>
skipList<K, V> ::~skipList(){
    if(_file_writer.is_open())
        _file_writer.close();
    if(_file_reader.is_open())
        _file_reader.close();

    Node<K, V>* current = _header -> forward[0];
    while(current){
        Node<K, V>* tmp = current -> forward[0];
        delete current;
        current = tmp;
    }
    delete _header;
}

template<typename K, typename V>
int skipList<K, V>::insert_element(K key, V val){
    mtx.lock();
    Node<K, V>* current = _header;
    Node<K, V>* update[_max_level + 1];    // update数组记录插入位置
    memset(update, 0, (_max_level + 1) * sizeof(Node<K, V>*));

    // 1. 先将 update[i] 记录好在每层应该插入的位置；
    for(int i = _skip_list_level; i >= 0; i--){
        while(current -> forward[i] && current -> forward[i] -> get_key() < key)
            current = current -> forward[i];
        update[i] = current;
    }
    // current即为小于 key的最大节点，其下一个节点为大于等于key的第一个节点。
    // 2. 对key的存在做判定，上面只是找到了应该插入的位置，但是不知道这个key是否存在
        // 2.1 key已经存在的话，直接返回
    current = current -> forward[0];     // reached level 0 and forward pointer to right node, which is desired to insert key.
    if(current && current -> get_key() == key){
        std::cout << "Key:  " << key << " has already existed." << std::endl;
        mtx.unlock();
        return 1;
    }
        // 2.2 key不存在，那么就确定要将此key插入多少层，裁定方法就是：
        // 抛硬币，得到正面向上次数 k，将跳表的前k层全部更新插入元素；若k大于最大层数，则新增一层，且更新头指针。
    int random_level = get_random_level(); 
    if(random_level > _skip_list_level){
        // 深度大于当前列表的深度，则更新深度，并更新update数组指向head。
        for(int i = _skip_list_level + 1; i <= random_level; ++i)
            update[i] = _header;
        _skip_list_level = random_level;
    }

    Node<K, V>* inserted_node = create_node(key, val, random_level);
    for(int i = random_level; i >= 0; --i){
        inserted_node -> forward[i] = update[i] -> forward[i];
        update[i] -> forward[i] = inserted_node;
    }

    std::cout << "successfully inserted key: " << key << ", value : " << val << std::endl;
    _element_count++;
    mtx.unlock();
    return 0;
}

template<typename K, typename V>
int skipList<K, V>::delete_element(K key){
    mtx.lock();
    Node<K, V>* current = _header;
    Node<K, V>* update[_max_level + 1];
    memset(update, 0, (_max_level + 1) * sizeof(Node<K, V>*));

    for(int i = _skip_list_level; i >= 0; --i){
        while(current->forward[i] && current -> forward[i] -> get_key() < key)
            current = current -> forward[i];
        update[i] = current;
    }
    // current即为小于 key的最大节点，其下一个节点为大于等于key的第一个节点。
    // 对key的存在做判定
    current = current -> forward[0];
    if(!current || current -> get_key() != key){
        std::cout << "Key: " << key << " do not exist." << std::endl;
        mtx.unlock();
        return 1;
    }

    for(int i = current ->node_level; i >= 0; --i){
        update[i] -> forward[i] = current -> forward[i];
    }
    while(_skip_list_level > 0 && _header -> forward[_skip_list_level] == 0)
        _skip_list_level--;
    
    delete current;
    std::cout << "successfully deleted key:  "  << key << std::endl;
    _element_count--;
    mtx.unlock();
    return 0;
}

template<typename K, typename V>
int skipList<K, V>::change_element(K key, V val){
    mtx.lock();
    Node<K, V>* current = _header;

    for(int i = _skip_list_level; i >= 0; --i){
        while(current -> forward[i] && current -> forward[i] -> get_key() < key){
            current = current -> forward[i];
        }
    }
    // current即为小于 key的最大节点，其下一个节点为大于等于key的第一个节点。
    // 对key的存在做判定
    current = current -> forward[0];
    if(!current || current -> get_key() != key){
        std::cout << "Key: " << key << "do not exist." << std::endl;
        mtx.unlock();
        return 1;
    }

    // current即为小于 key的最大节点，其下一个节点为大于等于key的第一个节点。
    // 对key的存在做判定
    if(current -> set_value(val)){
        std::cout << "Key: " << key << "change value failed." << std::endl;
        mtx.unlock();
        return 1;
    }

    std::cout << "successfully changed key: " << key << ", value :" << val << std::endl;
    mtx.unlock();
    return 0;
}

template<typename K, typename V>
bool skipList<K, V>::search_element(K key){
    Node<K, V>* current = _header;

    for(int i = _skip_list_level; i >= 0; --i){
        while(current -> forward[i] && current -> forward[i] -> get_key() < key){
            current = current -> forward[i];
        }
    }

    // current即为小于 key的最大节点，其下一个节点为大于等于key的第一个节点。
    // 对key的存在做判定
    current = current -> forward[0];
    if(current && current -> get_key() == key){
        std::cout << "Found key: " << key << ", value: " << current -> get_value() << std::endl;
        return true;
    }
    std::cout << "Not found Key: " << key << std::endl;
    return false;
}

template<typename K, typename V>
void skipList<K, V>::display_list(){
    std::cout << "\n-------SkipList is as played--------\n";
    for(int i = 0; i <= _skip_list_level; ++i){
        Node<K, V>* node = this -> _header -> forward[i];
        std::cout << "Level " << i << ":";
        while(node){
            std::cout << node -> get_key() << ":" << node -> get_value() << ";";
            node = node -> forward[i];
        }
        std::cout << std::endl;
    }
}

template<typename K, typename V>
void skipList<K, V>::dump_file(){
    std::cout << "\n -----Dump File -----\n";
    _file_writer.open(STORE_FILE);
    Node<K, V>* current = _header -> forward[0];

    while(current){
        _file_writer << current -> get_key() << delimiter << current -> get_value() << std::endl;
        std::cout << current -> get_key() << ":" << current -> get_value() << std::endl;
        current = current -> forward[0];
    }
    _file_writer.flush();
    _file_writer.close();
}

template<typename K, typename V>
void skipList<K, V>::load_file(){
    _file_reader.open(STORE_FILE);
    std::cout << "----load file----" << std::endl;
    std::string line, key, value;
    while(getline(_file_reader, line)){
        get_key_value_from_string(line, key, value);
        if(key.empty() || value.empty()){
            std::cout << "error data---- key: " << key << "value: " << value << std::endl;
            continue;
        }
        insert_element(key, value);
        std::cout << "Key: " << key << "value: " << value << std::endl; 
    }
    _file_reader.close();
}

template<typename K, typename V>
void skipList<K, V>::get_key_value_from_string(const std::string& str, std::string& key, const std::string& value){
    if(!is_valid_string(str))
        return;
    int index = str.find(delimiter);
    key = str.substr(0, index);
    value = str.substr(index + 1, str.size() - index - 1);
}

template<typename K, typename V>
bool skipList<K, V>::is_valid_string(const std::string& str){
    if(str.empty())
        return false;
    return str.find(delimiter) != std::string::npos;
}