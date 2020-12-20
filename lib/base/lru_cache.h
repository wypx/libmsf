#ifndef BASE_LRU_CACHE_H_
#define BASE_LRU_CACHE_H_

#include <linux/list.h>

#include <unordered_map>
#include <utility>

namespace MSF {

template <class _Key, class _Tp>
class LRUCache {
  struct _Node {
    std::pair<_Key, _Tp> data;
    struct list_head list;
  };

 public:
  // 常迭代器
  struct _List_const_iterator {
    typedef _List_const_iterator _Self;

    typedef std::ptrdiff_t difference_type;
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef std::pair<_Key, _Tp> value_type;
    typedef const std::pair<_Key, _Tp> *pointer;
    typedef const std::pair<_Key, _Tp> &reference;

    _List_const_iterator() : _M_node(nullptr) {}
    explicit _List_const_iterator(const _Node *__x) : _M_node(__x) {}

    reference operator*() const { return _M_node->data; }
    pointer operator->() const { return std::__addressof(_M_node->data); }

    _Self &operator++() {
      _M_node = ulist_next_entry(_M_node, list);
      return *this;
    }

    _Self operator++(int) {
      _Self __tmp = *this;
      _M_node = ulist_next_entry(_M_node, list);
      return __tmp;
    }

    _Self &operator--() {
      _M_node = ulist_prev_entry(_M_node, list);
      return *this;
    }

    _Self operator--(int) {
      _Self __tmp = *this;
      _M_node = ulist_prev_entry(_M_node, list);
      return __tmp;
    }

    bool operator==(const _Self &__x) const { return _M_node == __x._M_node; }
    bool operator!=(const _Self &__x) const { return _M_node != __x._M_node; }

    const _Node *_M_node;
  };

  typedef _Key key_type;
  typedef _Tp mapped_type;
  typedef std::pair<_Key, _Tp> data_type;
  typedef _Node value_type;
  typedef std::unordered_map<_Key, _Node> mapper_type;

  typedef _List_const_iterator const_iterator;

  explicit LRUCache(size_t __n, const mapped_type &__e = mapped_type())
      : size_(__n), err_({.data = {key_type(), __e}}), list_(&err_.list) {
    // 初始化队列
    INIT_LIST_HEAD(list_);
  }

  ~LRUCache() {}

  const mapped_type &Get(const key_type &__k) {
    const_iterator __i = find(__k);
    return __i->second;
  }

  void Put(const key_type &__k, const mapped_type &__v) {
    auto __i = find(__k);
    if (__i == end()) {
      insert(data_type(__k, __v));
      return;
    }
    value_type *__e = const_cast<value_type *>(__i._M_node);
    __e->data.second = __v;
    __move_to_head(__e);
  }

  const mapped_type &NotFound() const {
    const_iterator __i = end();
    return __i->second;
  }

  /**
   * 迭代器
   */
  const_iterator begin() const {
    value_type *__e = ulist_first_entry(list_, value_type, list);
    return const_iterator(__e);
  }

  const_iterator end() const { return const_iterator(&err_); }

  /**
   * 容量
   */
  bool empty() const { return map_.empty(); }
  size_t size() const { return map_.size(); }
  size_t max_size() const { return size_; }

  /**
   * 修改器
   */
  void clear() {
    map_.clear();
    INIT_LIST_HEAD(list_);
  }

  std::pair<const_iterator, bool> insert(const data_type &__d) {
    auto __i = map_.find(__d.first);
    if (__i != map_.end()) {
      return std::make_pair(const_iterator(&__i->second), false);
    }
    // 插入新节点
    __i = map_.emplace(__d.first, value_type{.data = __d}).first;
    value_type &__e = __i->second;
    ulist_add(&__e.list, list_);
    // 修剪缓存大小
    __trim_to_size();
    return std::make_pair(const_iterator(&__i->second), true);
  }

  void erase(const_iterator pos) {
    assert(contains(pos->first));
    __erase(pos._M_node);
  }

  size_t erase(const key_type &__k) {
    auto __i = map_.find(__k);
    if (__i == map_.end()) {
      return 0;
    }
    __erase(&__i->second);
    return 1;
  }

  void resize(size_t __n) {
    size_ = __n;
    __trim_to_size();
  }

  /**
   * 查找
   */
  bool contains(const key_type &__k) const {
    return map_.find(__k) != map_.end();
  }

  const_iterator find(const key_type &__k) {
    auto __i = map_.find(__k);
    if (__i == map_.end()) {
      return end();
    }
    // 移动到缓存队列开头
    __move_to_head(&__i->second);
    return const_iterator(&__i->second);
  }

 private:
  void __erase(value_type *__e) {
    // 将节点从队列移除
    ulist_del(&__e->list);
    // 将节点从映射表移除
    map_.erase(__e->data.first);
  }

  void __move_to_head(value_type *__e) {
    // 从队列中移除
    ulist_del(&__e->list);
    // 放入队列头部
    ulist_add(&__e->list, list_);
  }

  void __trim_to_size() {
    while (map_.size() > size_) {
      // 找到队列最后一个节点
      value_type *__e = ulist_last_entry(list_, value_type, list);
      // 移除节点
      __erase(__e);
    }
  }

  size_t size_;             // 缓存大小
  value_type err_;          // 未命中缓存时返回的值
  struct list_head *list_;  // 队列
  mapper_type map_;         // 映射表
};

}  // namespace MSF

#endif
