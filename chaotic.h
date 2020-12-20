/*
 * Copyright (c) Konstantin Hamidullin. All rights reserved.
 */

namespace chaotic {

size_t char_to_size(char ch) {
    return static_cast<size_t>(ch);
}

class Matrix {
public:
    static constexpr char EMPTY = ' ';

    class Reference {
    public:
        Reference(): _symbol(EMPTY), _counter(0) {
        }
        Reference(char ch): _symbol(ch), _counter(0) {
        }
        bool is_compatible(char ch) const {
            return (_counter == 0) || (ch == _symbol);
        }
        char &symbol() {
            return _symbol;
        }
        void inc() {
            _counter++;
        }
        void dec() {
            _counter--;
        }
        bool is_null() const {
            return (_counter == 0);
        }
    private:
        char    _symbol;
        size_t  _counter;
    };
    Matrix() {
    }

    bool can_add(char clear, char cipher) {
        bool a = _next[char_to_size(clear)].is_compatible(cipher);
        bool b = _prev[char_to_size(cipher)].is_compatible(clear);
        return a && b;
    }

    void add(char clear, char cipher) {
        swap(clear, cipher);
        inc_refs(cipher, clear);
        //remove(clear, cipher);
    }
    void remove(char clear, char cipher) {
        dec_refs(cipher, clear);
        swap(cipher, clear);
    }
private:
    void swap(char a, char b) {
        std::swap(_prev[char_to_size(a)], _prev[char_to_size(b)]);
        std::swap(_next[char_to_size(a)], _next[char_to_size(b)]);
        _prev[char_to_size(a)].symbol() = b;
        _next[char_to_size(b)].symbol() = a;

        if (!_prev[char_to_size(b)].is_null()) {
            char prev = _prev[char_to_size(b)].symbol();
            _next[char_to_size(prev)].symbol() = b;
        }

        if (!_next[char_to_size(a)].is_null()) {
            char next = _next[char_to_size(a)].symbol();
            _prev[char_to_size(next)].symbol() = a;
        }
    }
    void set_refs(char prev, char next) {
        _next[char_to_size(prev)].symbol() = next;
        _prev[char_to_size(next)].symbol() = prev;
    }
    void inc_refs(char prev, char next) {
        _next[char_to_size(prev)].inc();
        _prev[char_to_size(next)].inc();
    }
    void dec_refs(char prev, char next) {
        _next[char_to_size(prev)].dec();
        _prev[char_to_size(next)].dec();
    }

    std::array<Reference, 128> _next, _prev;
};

class Chaotic {
public:
    Chaotic()
    {
    }
    std::string key() const {
        return "";
    }
    bool push(const std::string &clear, const std::string &cipher, char ch) {
        if (ch == cipher[clear.size()]) {
            return false;
        }
        if (_matrix.can_add(ch, cipher[clear.size()])) {
            _matrix.add(ch, cipher[clear.size()]);
            return true;
        }
        else {
            return false;
        }
    }
    void pop(const std::string &clear, const std::string &cipher, char ch) {
        _matrix.remove(ch, cipher[clear.size()]);
    }
    template <class _Next>
    void test(const std::string &, const std::string &, const _Next &next) {
        next();
    }
private:
    Matrix      _matrix;
};

}
