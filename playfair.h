/*
 * Copyright (c) Konstantin Hamidullin. All rights reserved.
 */

namespace playfair {

constexpr size_t UNSET = 255;
constexpr size_t MATRIX_SIDE_SIZE = 5;
constexpr size_t MATRIX_SIZE = MATRIX_SIDE_SIZE * MATRIX_SIDE_SIZE;

size_t char_to_size(char ch) {
    return static_cast<size_t>(ch);
}

using Char_Pair = std::pair<char, char>;

bool cross_equal(const Char_Pair &a, const Char_Pair &b) {
    return (a.first == b.second) && (a.second == b.first);
}

class Char_Unit {
public:
    Char_Unit(const Char_Pair &clear, const Char_Pair &cipher) noexcept:
    _clear_text(clear), _cipher_text(cipher) {
    }

    const Char_Pair &clear_text() const {
        return _clear_text;
    }
    const Char_Pair &cipher_text() const {
        return _cipher_text;
    }
    bool is_triplet() const {
        return (_clear_text.first == _cipher_text.second) || (_clear_text.second == _cipher_text.first);
    }
    bool compatible(const Char_Unit &that) const {
        if (_clear_text == that._clear_text) {
            return (_cipher_text == that._cipher_text);
        }
        else if (_cipher_text == that._cipher_text) {
            return false; // due to previous condition is false
        }
        else if (cross_equal(_clear_text, that._clear_text)) {
            return cross_equal(_cipher_text, that._cipher_text);
        }
        else if (cross_equal(_cipher_text, that._cipher_text)) {
            return false;
        }
        else if (_clear_text == that._cipher_text) {
            return test_same_clear_cipher(that);
        }
        else if (that._clear_text == _cipher_text) {
            return that.test_same_clear_cipher(*this);
        }
        else if (cross_equal(_clear_text, that._cipher_text)) {
            return test_same_clear_cipher(that.inverse());
        }
        else if (cross_equal(that._clear_text, _cipher_text)) {
            return that.test_same_clear_cipher(inverse());
        }
        else {
            return true;
        }
    }
    bool same(const Char_Unit &that) const {
        return ((_clear_text == that._clear_text) && (_cipher_text == that._cipher_text)) ||
            (cross_equal(_clear_text, that._clear_text) && cross_equal(_cipher_text, that._cipher_text));
    }
    Char_Unit inverse() const {
        return Char_Unit({_clear_text.second, _clear_text.first}, {_cipher_text.second, _cipher_text.first});
    }
private:
    bool test_same_clear_cipher(const Char_Unit &that) const {
        // given: _clear_text == that._cipher_text
        return (_cipher_text == that._clear_text) || same_line(that) || inverse().same_line(that.inverse());
    }
    bool same_line(const Char_Unit &that) const {
        // given: _clear_text == that._cipher_text
        // 4-letter
        // AB DA
        // BC AB
        bool b1 = (_cipher_text.first == _clear_text.second) && (that._cipher_text.first == that._clear_text.second) &&
            (_cipher_text.second != _clear_text.first) && (_cipher_text.second != _clear_text.second) &&
            (that._clear_text.first != that._cipher_text.first) && (that._clear_text.first != that._cipher_text.second) &&
            (_cipher_text.second != that._clear_text.first);
        //5-letter
        // AB EC
        // CD AB
        bool b2 = (_cipher_text.first == that._clear_text.second) &&
            (_cipher_text.first != _clear_text.first) && (_cipher_text.first != _clear_text.second) &&
            (_cipher_text.second != _clear_text.first) && (_cipher_text.second != _clear_text.second) && (_cipher_text.second != _cipher_text.first) &&
            (that._clear_text.first != that._cipher_text.first) && (that._clear_text.first != that._cipher_text.second) && (that._clear_text.first != that._clear_text.second) &&
            (_cipher_text.second != that._clear_text.first);
        return b1 || b2;
    }
    Char_Pair   _clear_text;
    Char_Pair   _cipher_text;
};

class Matrix {
public:
    static constexpr char EMPTY = ' ';
    Matrix(const std::string &val): _val(val), _rev() {
        for(size_t i = 0; i < _val.size(); ++i) {
            _rev[char_to_size(_val[i])] = i;
        }
    }
    Matrix(): _val(MATRIX_SIZE, EMPTY), _rev() {
        for(auto &w: _rev) {
            w = UNSET;
        }
    }
    const std::string &val() const {
        return _val;
    }
    char val(size_t n) const {
        return _val[n];
    }
    size_t rev(char ch) const {
        return _rev[char_to_size(ch)];
    }
    void add(size_t n, char ch) {
        _val[n] = ch;
        _rev[char_to_size(ch)] = n;
    }
    void remove(size_t n, char ch) {
        _val[n] = EMPTY;
        _rev[char_to_size(ch)] = UNSET;
    }
private:
    std::string     _val;
    std::array<size_t, 128> _rev;
};

using Position_List = std::vector<size_t>;

bool find_position(const Position_List &positions, size_t pos) {
    return (std::find(positions.begin(), positions.end(), pos) != positions.end());
}

class Rules {
public:
    Rules() {
        for(size_t a = 0; a < MATRIX_SIZE; ++a) {
            _none_list.push_back(a);
        }
        for(size_t a = 0; a < MATRIX_SIZE; ++a) {
            for(size_t b = 0; b < MATRIX_SIZE; ++b) {
                if (a != b) {
                    size_t ax = a % MATRIX_SIDE_SIZE;
                    size_t ay = a / MATRIX_SIDE_SIZE;
                    size_t bx = b % MATRIX_SIDE_SIZE;
                    size_t by = b / MATRIX_SIDE_SIZE;
                    if (ax == bx) {
                        // same column
                        ay = (ay + 1) % MATRIX_SIDE_SIZE;
                        by = (by + 1) % MATRIX_SIDE_SIZE;
                    }
                    else if (ay == by) {
                        // same row
                        ax = (ax + 1) % MATRIX_SIDE_SIZE;
                        bx = (bx + 1) % MATRIX_SIDE_SIZE;
                    }
                    else {
                        std::swap(ax, bx);
                    }
                    size_t na = ay * MATRIX_SIDE_SIZE + ax;
                    size_t nb = by * MATRIX_SIDE_SIZE + bx;
                    _change[a][b] = {na, nb};
                    _rchange[na][nb] = {a, b};

                    _both_list[a][b] = {na};
                    _rboth_list[na][nb] = {a};

                    _self_list[a].push_back(na);
                    _rself_list[na].push_back(a);

                    _opp_list[b].push_back(na);
                    _ropp_list[na].push_back(b);
                }
            }
        }


        for(auto &v: _self_list) {
            make_unique(v);
        }
        for(auto &v: _rself_list) {
            make_unique(v);
        }

        for(auto &v: _opp_list) {
            make_unique(v);
        }
        for(auto &v: _ropp_list) {
            make_unique(v);
        }
    }
    void make_unique(Position_List &list) {
        std::sort(list.begin(), list.end());
        list.erase(std::unique(list.begin(), list.end()), list.end());
    }
    const Position_List &get_rpositions(char ch1, char ch2, const Matrix &m) const {
        size_t p1 = m.rev(ch1);
        size_t p2 = m.rev(ch2);
        bool b1 = (p1 != UNSET);
        bool b2 = (p2 != UNSET);

        const Position_List &positions =
            ((b1 && b2) ? _rboth_list[p1][p2] :
            (b1 ? _rself_list[p1] :
            (b2 ? _ropp_list[p2] : _none_list)));
        return positions;
    }
    std::pair<size_t, size_t> change(size_t n1, size_t n2) const {
        return _change[n1][n2];
    }
    std::pair<size_t, size_t> rchange(size_t n1, size_t n2) const {
        return _rchange[n1][n2];
    }
private:
    std::array<std::array<std::pair<size_t, size_t>, MATRIX_SIZE>, MATRIX_SIZE>   _change, _rchange;
    std::array<Position_List, MATRIX_SIZE> _self_list, _rself_list;
    std::array<Position_List, MATRIX_SIZE> _opp_list, _ropp_list;
    Position_List       _none_list;
    std::array<std::array<Position_List, MATRIX_SIZE>, MATRIX_SIZE>   _both_list, _rboth_list;
};

class Playfair {
public:
    Playfair(size_t matrix_creation_point):
    _rules(), _matrix(),
    _matrix_creation_point(matrix_creation_point),
    _i_clear(0), _i_cipher(0), _char_freq(), _char_set()
    {
    }
    const std::string &key() const {
        return _matrix.val();
    }
    bool push(const std::string &clear, const std::string &cipher, char ch) {
        if (ch == cipher[clear.size()]) {
            return false;
        }
        if (clear.size() % 2 == 1) {
            if (clear.back() == ch) {
                return false;
            }
        }
        if (clear.size() % 2 == 1) {
            size_t s = clear.size();
            char ch1 = clear[s - 1];
            char w1 = cipher[s - 1];
            char w2 = cipher[s];
            Char_Unit u({ch1, ch}, {w1, w2});
            if (cross_equal(u.clear_text(), u.cipher_text())) {
                return false;
            }
            for(const Char_Unit &w: _units) {
                if (!w.compatible(u)) {
                    return false;
                }
            }
            _units.push_back(u);
        }
        return true;
    }
    void pop(const std::string &clear, const std::string &, char) {
        if (clear.size() % 2 == 1) {
            _units.pop_back();
        }
    }
    template <class _Next>
    void test(const std::string &clear, const std::string &cipher, const _Next &next) {
        if ((_i_cipher + 1) * 2 <= _i_clear) {
            Char_Unit u = as_char_unit(clear, cipher, _i_cipher);
            _i_cipher++;
            set_cipher(u, [&]() {
                test(clear, cipher, next);
                return false;
            });
            _i_cipher--;
        }
        else if ((_i_clear == 0) && ((clear.size() >= 16) || ((clear.size() >= 6) && (clear.size() % 2 == 0)))) {
            set_clear_set(clear, cipher, [&]() {
                test(clear, cipher, next);
                return false;
            },
            [&](){
                next();
            });
        }
        else if ((_i_clear > 0) && (_i_clear < clear.size())) {
            const Char_Unit &u = as_char_unit(clear, cipher, _i_cipher); // out of range here
            bool even = (_i_clear % 2 == 0);
            _i_clear++;
            set_clear(u, even, [&]() {
                test(clear, cipher, next);
                return false;
            });
            _i_clear--;
        }
        else {
            next();
        }
    }
private:
    size_t &char_freq(char ch) {
        return _char_freq[char_to_size(ch)];
    }
    void set_char_set(char ch) {
        size_t s = char_to_size(ch);
        if (!_char_set[s]) {
            _char_set[s] = true;
            _char_unique++;
        }
    }
    const bool &char_set(char ch) const {
        return _char_set[char_to_size(ch)];
    }
    void clear_char_info() {
        _char_unique = 0;
        for(size_t ch = char_to_size('a'); ch <= char_to_size('z'); ++ch) {
            _char_freq[ch] = 0;
            _char_set[ch] = false;
        }
    }
    Char_Unit as_char_unit(const std::string &clear, const std::string cipher, size_t n) const {
        char ch1 = clear[2 * n];
        char ch2 = clear[2 * n + 1];
        char w1 = cipher[2 * n];
        char w2 = cipher[2 * n + 1];
        return Char_Unit({ch1, ch2}, {w1, w2});
    }

    template <class _F>
    bool _process_set(const std::vector<Char_Unit> &units, size_t i_unit, const _F &f) {
        if (i_unit < units.size()) {
            const Char_Unit &u = units[i_unit];
            return set_clear(u, true, [&]() {
                return set_clear(u, false, [&]() {
                    return set_cipher(u, [&]() {
                       return _process_set(units, i_unit + 1, f);
                    });
                });
            });
        }
        else {
            return f();
        }
    }

    template <class _F>
    bool process_set(const std::vector<Char_Unit> &_units, const _F &f) {
        _matrix.add(0, _units[0].clear_text().first);
        bool result = _process_set(_units, 0, f);
        _matrix.remove(0, _units[0].clear_text().first);
        return result;
    }

    template <class _F>
    void print_units(const std::vector<Char_Unit> &list, const std::vector<Char_Unit>::const_iterator &mark, const _F &f) {
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it == mark) {
                std::cout << "## ";
            }
            const Char_Unit &u = *it;
            std::cout << u.clear_text().first << u.clear_text().second;
            std::cout << "*";
            std::cout << u.cipher_text().first << u.cipher_text().second;
            std::cout << (u.is_triplet() ? "+" : "-") << f(u);
            std::cout << "  ";
        }
        if (list.end() == mark) {
            std::cout << "## ";
        }
        std::cout << std::endl;
    }

    template <class _F>
    bool set_cipher(const Char_Unit &unit, const _F &f) {
        size_t p1 = _matrix.rev(unit.clear_text().first);
        size_t p2 = _matrix.rev(unit.clear_text().second);

        auto p = _rules.change(p1, p2);
        bool e1 = ((_matrix.val(p.first) == Matrix::EMPTY) && (_matrix.rev(unit.cipher_text().first) == UNSET));
        bool e2 = ((_matrix.val(p.second) == Matrix::EMPTY) && (_matrix.rev(unit.cipher_text().second) == UNSET));
        if (e1 || (_matrix.val(p.first) == unit.cipher_text().first)) {
            if (e2 || (_matrix.val(p.second) == unit.cipher_text().second)) {
                if (e1) {
                    _matrix.add(p.first, unit.cipher_text().first);
                }
                if (e2) {
                    _matrix.add(p.second, unit.cipher_text().second);
                }
                bool result = f();
                if (e1) {
                    _matrix.remove(p.first, unit.cipher_text().first);
                }
                if (e2) {
                    _matrix.remove(p.second, unit.cipher_text().second);
                }
                return result;
            }
        }
        return false;
    }

    template <class _F>
    bool set_clear(const Char_Unit &unit, bool even, const _F &f) {
        const Position_List &positions = even ?
            _rules.get_rpositions(unit.cipher_text().first, unit.cipher_text().second, _matrix) :
            _rules.get_rpositions(unit.cipher_text().second, unit.cipher_text().first, _matrix);
        char ch = even ? unit.clear_text().first : unit.clear_text().second;
        if (_matrix.rev(ch) == UNSET) {
            for(size_t p: positions) {
                if (_matrix.val(p) == Matrix::EMPTY) {
                    _matrix.add(p, ch);
                    bool result = f();
                    _matrix.remove(p, ch);
                    if (result) {
                        return true;
                    }
                }
            }
            return false;
        }
        else if (find_position(positions, _matrix.rev(ch))) {
            return f();
        }
        return false;
    }

    template <class _F, class _N>
    void set_clear_set(const std::string &clear, const std::string &cipher, const _F &f, const _N &n) {
        clear_char_info();
        _units_sorted.clear();
        for(const Char_Unit &u: _units) {
            bool add = true;
            for(const Char_Unit &w: _units_sorted) {
                if (w.same(u)) {
                    add = false;
                    break;
                }
            }
            if (add) {
                _units_sorted.push_back(u);
                char_freq(u.clear_text().first)++;
                char_freq(u.clear_text().second)++;
                char_freq(u.cipher_text().first)++;
                char_freq(u.cipher_text().second)++;
            }
        }
        auto unit_freq = [&] (const Char_Unit &u) {
            size_t f1 = char_freq(u.clear_text().first) + char_freq(u.clear_text().second);
            size_t f2 = char_freq(u.cipher_text().first) + char_freq(u.cipher_text().second);
            return f1 + f2;
        };

        auto sort_units = [&] (auto begin, auto end) {
            std::sort(begin, end, [&](const Char_Unit &a, const Char_Unit &b) {
                return unit_freq(a) > unit_freq(b);
            });
        };

        auto cur = _units_sorted.begin();
        auto end = _units_sorted.end();
        sort_units(cur, end);
        /**/
        auto move_first = [&] (auto it) {
            std::swap(*cur, *it);

            char_freq(cur->clear_text().first)--;
            char_freq(cur->clear_text().second)--;
            char_freq(cur->cipher_text().first)--;
            char_freq(cur->cipher_text().second)--;

            set_char_set(cur->clear_text().first);
            set_char_set(cur->clear_text().second);
            set_char_set(cur->cipher_text().first);
            set_char_set(cur->cipher_text().second);
        };

        //std::cout << "<<" << text.value << ">>" << std::endl;
        //print_units(_units, cur, unit_freq);
        size_t mode = 4;
        while (cur != end) {
            //std::cout << "mode " << mode << std::endl;
            bool moved = false;
            if (mode == 0) {
                // search units with both cipher or both clear text chars set
                // or "triplets" with two different chars set
                for(auto it = cur; (it != end) && !moved; ++it) {
                    bool clear =
                        char_set(it->clear_text().first) &&
                        char_set(it->clear_text().second);
                    bool cipher =
                        char_set(it->cipher_text().first) &&
                        char_set(it->cipher_text().second);

                    if (clear || cipher) {
                        move_first(it);
                        moved = true;
                    }
                    else if (it->is_triplet()) {
                        bool c1 =
                            char_set(it->clear_text().first) &&
                            char_set(it->cipher_text().second) &&
                            (it->clear_text().first != it->cipher_text().second);
                        bool c2 =
                            char_set(it->clear_text().second) &&
                            char_set(it->cipher_text().first) &&
                            (it->clear_text().second != it->cipher_text().first);
                        if (c1 || c2) {
                            move_first(it);
                            moved = true;
                        }
                    }
                }
            }
            else if (mode == 1) {
                // search "triplets" with one char set
                for(auto it = cur; (it != end) && !moved; ++it) {
                    bool clear =
                        char_set(it->clear_text().first) ||
                        char_set(it->clear_text().second);
                    bool cipher =
                        char_set(it->cipher_text().first) ||
                        char_set(it->cipher_text().second);
                    if (it->is_triplet() && (clear || cipher)) {
                        move_first(it);
                        moved = true;
                    }
                }
            }
            else if (mode == 2) {
                // search units with 2 or more chars set
                for(auto it = cur; (it != end) && !moved; ++it) {
                    bool clear =
                        char_set(it->clear_text().first) ||
                        char_set(it->clear_text().second);
                    bool cipher =
                        char_set(it->cipher_text().first) ||
                        char_set(it->cipher_text().second);
                    if (clear && cipher) {
                        move_first(it);
                        moved = true;
                    }
                }
            }
            else if (mode == 3) {
                // search units with one char set
                for(auto it = cur; (it != end) && !moved; ++it) {
                    bool c1 =
                        char_set(it->cipher_text().first) ||
                        char_set(it->clear_text().second);
                    bool c2 =
                        char_set(it->clear_text().first) ||
                        char_set(it->cipher_text().second);

                    if (c1 || c2) {
                        move_first(it);
                        moved = true;
                    }
                }
            }
            else if (mode == 4) {
                // search "triplets"
                for(auto it = cur; (it != end) && !moved; ++it) {
                    if (it->is_triplet()) {
                        move_first(it);
                        moved = true;
                    }
                }
            }
            else if (mode == 5) {
                // take the first unit
                move_first(cur);
                moved = true;
            }
            if (moved) {
                ++cur;
                mode = 0;
                sort_units(cur, end);
                //print_units(_units, cur, unit_freq);
            }
            else {
                mode = mode + 1;
            }
        }
        /**/

        bool ok = process_set(_units_sorted, [&]() {
            if (clear.size() % 2 == 0) {
                return true;
            }
            const Char_Unit &u = as_char_unit(clear, cipher, clear.size() / 2); // out of range here
            return set_clear(u, true, [&]() {
                return true;
            });
        });

        if (!ok) {
            return;
        }
        if (clear.size() < _matrix_creation_point) {
            n();
        }
        else {
            _i_cipher = _units.size();
            _i_clear = _units.size() * 2;
            process_set(_units_sorted, f);
            _i_cipher = 0;
            _i_clear = 0;
        }
    }

    Rules               _rules;
    Matrix              _matrix;
    size_t              _matrix_creation_point;

    size_t                      _i_clear, _i_cipher;
    std::array<size_t, 128>     _char_freq;
    std::array<bool, 128>       _char_set;
    size_t                      _char_unique;
    std::vector<Char_Unit>      _units_sorted;
    std::vector<Char_Unit>      _units;
};

}
