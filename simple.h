/*
 * Copyright (c) Konstantin Hamidullin. All rights reserved.
 */

namespace simple {

size_t char_to_size(char ch) {
    return static_cast<size_t>(ch);
}

template <class _Symbol>
class Reference {
public:
    Reference(): _symbol(), _counter(0) {
    }
    bool is_compatible(_Symbol ch) const {
        return (_counter == 0) || (ch == _symbol);
    }
    void inc(_Symbol ch) {
        _symbol = ch;
        _counter++;
    }
    void dec() {
        _counter--;
    }
    bool is_null() const {
        return (_counter == 0);
    }
private:
    _Symbol    _symbol;
    size_t  _counter;
};

class Simple {
public:
    Simple(): _sub(), _inv()
    {
    }
    std::string key() const {
        return "";
    }
    bool push(const std::string &clear, const std::string &cipher, char ch) {
        char w = cipher[clear.size()];
        bool a = _sub[char_to_size(ch)].is_compatible(w);
        bool b = _inv[char_to_size(w)].is_compatible(ch);
        if (a && b) {
            _sub[char_to_size(ch)].inc(w);
            _inv[char_to_size(w)].inc(ch);
            return true;
        }
        else {
            return false;
        }
    }
    void pop(const std::string &clear, const std::string &cipher, char ch) {
        char w = cipher[clear.size()];
        _sub[char_to_size(ch)].dec();
        _inv[char_to_size(w)].dec();
    }
    template <class _Next>
    void test(const std::string &, const std::string &, const _Next &next) {
        next();
    }
private:
    std::array<Reference<char>, 128> _sub, _inv;
};

class Bigram {
public:
    using Symbol_Type = std::pair<char, char>;
    Bigram(): _sub(), _inv()
    {
    }
    std::string key() const {
        return "";
    }
    bool push(const std::string &clear, const std::string &cipher, char ch) {
        if (clear.size() % 2 == 0) {
            return true;
        }
        char w1 = cipher[clear.size() - 1];
        char w2 = cipher[clear.size()];
        bool a = _sub[char_to_size(clear.back())][char_to_size(ch)].is_compatible({w1, w2});
        bool b = _inv[char_to_size(w1)][char_to_size(w2)].is_compatible({clear.back(), ch});
        if (a && b) {
            _sub[char_to_size(clear.back())][char_to_size(ch)].inc({w1, w2});
            _inv[char_to_size(w1)][char_to_size(w2)].inc({clear.back(), ch});
            return true;
        }
        else {
            return false;
        }
    }
    void pop(const std::string &clear, const std::string &cipher, char ch) {
        if (clear.size() % 2 == 1) {
            char w1 = cipher[clear.size() - 1];
            char w2 = cipher[clear.size()];
            _sub[char_to_size(clear.back())][char_to_size(ch)].dec();
            _inv[char_to_size(w1)][char_to_size(w2)].dec();
        }
    }
    template <class _Next>
    void test(const std::string &, const std::string &, const _Next &next) {
        next();
    }
private:
    std::array<std::array<Reference<Symbol_Type>, 128>, 128> _sub, _inv;
};

class Pelling {
public:
    Pelling(size_t count): _count(count),
    _sub(count, std::array<Reference<char>, 128>()), _inv(count, std::array<Reference<char>, 128>())
    {
    }
    std::string key() const {
        return "";
    }
    bool push(const std::string &clear, const std::string &cipher, char ch) {
        std::array<Reference<char>, 128> &sub = _sub[clear.size() % _count];
        std::array<Reference<char>, 128> &inv = _inv[clear.size() % _count];
        char w = cipher[clear.size()];
        bool a = sub[char_to_size(ch)].is_compatible(w);
        bool b = inv[char_to_size(w)].is_compatible(ch);
        if (a && b) {
            sub[char_to_size(ch)].inc(w);
            inv[char_to_size(w)].inc(ch);
            return true;
        }
        else {
            return false;
        }
    }
    void pop(const std::string &clear, const std::string &cipher, char ch) {
        std::array<Reference<char>, 128> &sub = _sub[clear.size() % _count];
        std::array<Reference<char>, 128> &inv = _inv[clear.size() % _count];
        char w = cipher[clear.size()];
        sub[char_to_size(ch)].dec();
        inv[char_to_size(w)].dec();
    }
    template <class _Next>
    void test(const std::string &, const std::string &, const _Next &next) {
        next();
    }
private:
    size_t _count;
    std::vector<std::array<Reference<char>, 128>> _sub, _inv;
};

}
