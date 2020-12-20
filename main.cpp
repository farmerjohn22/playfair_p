/*
 * Copyright (c) Konstantin Hamidullin. All rights reserved.
 */
#include <iostream>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <string>
#include <cmath>
#include <future>
#include <assert.h>
#include <dict.h>
#include <simple.h>
#include <playfair.h>
#include <chaotic.h>

class Result {
public:
    using Result_List = std::map<score_t, std::set<Word_List>>;

    Result(const Word_Id_Map &word_id_map, size_t low_score_area, score_t low_score_limit,  score_t high_score_limit, size_t print_solutions):
    _start(std::chrono::steady_clock::now()), _word_id_map(word_id_map),
    _low_score_area(low_score_area), _low_score_limit(low_score_limit), _high_score_limit(high_score_limit),
    _print_solutions(print_solutions), _best_size(0) {
    }
    size_t low_score_area() const {
        return _low_score_area;
    }
    score_t low_score_limit() const {
        return _low_score_limit;
    }
    score_t high_score_limit() const {
        return _high_score_limit;
    }
    template <class _Solution>
    void add_to_list(const std::string &name, Result_List &list, const std::string &text, score_t score, const _Solution &solution, const Word_List &words) {
        if (list[score].insert(words).second) {
            bool list_updated = (score <= last_printed(list, false));
            if ((_print_solutions >= 2) || ((_print_solutions >= 1) && list_updated)) {
                print_time();
                std::cout << "  " << name << ": " << text.size() << " (";
                std::cout << _low_score_area << "/" << score_to_str(_low_score_limit) << "/" << score_to_str(_high_score_limit);
                std::cout << ")" << std::endl;
                std::cout << "  " << text << std::endl;
                std::cout << "  (" << score_to_str(score) << "): ";
                print_words(words);
                std::cout << std::endl;
                std::cout << "  =" << solution.key() << "=" << std::endl;
            }
            if (list_updated) {
                print_result_list(name, list, false);
            }
        }
    }
    template <class _Solution>
    void test_best(const std::string &text, score_t score, const _Solution &solution, const Word_List &words) {
        std::lock_guard<std::mutex> lock(_mtx);
        add_to_list("Solution", _best_list, text, score, solution, words);
    }
    template <class _Solution>
    void test_better(const std::string &text, score_t score, const _Solution &solution, const Word_List &words) {
        if ((_print_solutions >= 3) && (text.size() > _best_size)) {
            std::lock_guard<std::mutex> lock(_mtx);
            _best_size = text.size();
            print_time();
            std::cout << " Improvement: " << _best_size << " (";
            std::cout << _low_score_area << "/" << score_to_str(_low_score_limit) << "/" << score_to_str(_high_score_limit);
            std::cout << ")" << std::endl;
            std::cout << "  " << text << std::endl;
            std::cout << "  (" << score<< "): ";
            print_words(words);
            std::cout << std::endl;
            std::cout << "  =" << solution.key() << "=" << std::endl;
        }
    }
    void print_state(size_t t, const std::string &s, size_t n, size_t total) {
        std::lock_guard<std::mutex> lock(_mtx);
        print_time();
        std::cout << " t" << t << ": " << s << " (" << n << "/" << total << ")" << std::endl;
    }
    void print_result_lists(bool final) {
        print_result_list("Best", _best_list, final);
    }
    void print_time() const {
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _start);
        std::cout << "[" << d.count() << "]";
    }

    score_t last_printed(const Result_List &list, bool final) const {
        size_t max_print = final ? MAX_FINAL_PRINT : MAX_CURRENT_PRINT;
        size_t printed = 0;
        score_t result = 0;
        for(const auto &bs: list) {
            if (printed < max_print) {
                printed += bs.second.size();
                result = bs.first;
            }
        }
        return result;
    }
    void print_result_list(const std::string &name, const Result_List &list, bool final) {
        size_t max_print = final ? MAX_FINAL_PRINT : MAX_CURRENT_PRINT;
        size_t printed = 0, total = 0;
        for(const auto &bs: list) {
            if (printed < max_print) {
                printed += bs.second.size();
            }
            total += bs.second.size();
        }

        print_time();
        std::cout << "  " << name;
        if (final) {
            std::cout << " final ";
        }
        else {
            std::cout << " current top ";
        }
        std::cout << printed << " result(s)";
        if (printed != total) {
            std::cout << " of " << total;
        }
        std::cout << " (";
        std::cout << _low_score_area << "/" << score_to_str(_low_score_limit) << "/" << score_to_str(_high_score_limit);
        std::cout << "):" << std::endl;
        size_t p = 0;
        for(const auto &bs: list) {
            if (p < printed) {
                for(const auto &wl: bs.second) {
                    std::cout << "  (" << score_to_str(bs.first) << "): ";
                    for(auto w: wl) {
                        std::cout << _word_id_map.word_by_id(w.id()) << " ";
                    }
                    std::cout << std::endl;
                }
                p += bs.second.size();
            }
            else {
                break;
            }
        }
    }
private:
    void print_words(const Word_List &words) const {
        for(auto w: words) {
            std::cout << _word_id_map.word_by_id(w.id()) << "(" << score_to_str(w.score());
            if (w.category() > 0) {
                std::cout << "+" << score_to_str(w.category());
                if (_word_id_map.category(w.id()) == PROPER) {
                    std::cout << "p";
                }
                else if (_word_id_map.category(w.id()) == NUMERIC) {
                    std::cout << "u";
                }
            }
            if (w.other() > w.score()) {
                std::cout << "|" << score_to_str(w.other()) << "o";
            }
            std::cout << ") ";
        }
    }
    Ticks               _start;
    const Word_Id_Map   &_word_id_map;
    size_t              _low_score_area;
    score_t             _low_score_limit;
    score_t             _high_score_limit;
    size_t              _print_solutions;
    size_t              _best_size;
    Result_List         _best_list;
    std::mutex          _mtx;
};

template <class _Matcher>
class Search {
public:
    Search(const _Matcher &matcher, const Dictionary &dict, Result &result, const std::string &cipher, bool odd_mode, bool use_comma_start, bool use_comma_inside, char filler):
    _matcher(matcher), _dict(dict), _result(result), _clear_fixed(), _clear(),
    _score(0), _score_category(0), _score_other(0),
    _cipher(cipher),
    _odd_mode(odd_mode), _use_comma_start(use_comma_start), _use_comma_inside(use_comma_inside), _filler(filler)
    {
    }
    void operator()(const std::string &fixed) {
        _clear_fixed = fixed;

        if (_use_comma_start) {
            _words.emplace_back(COMMA, 0, 0, 0);
        }

        if (_odd_mode) {
            char first = _clear_fixed.empty() ? Prefix_Tree::EMPTY : _clear_fixed.front();
            if (!_clear_fixed.empty()) {
                _clear_fixed = _clear_fixed.substr(1);
            }

            const Word_Ngram_Tree &ngt = _dict.word_ngram_tree();
            const Prefix_Tree &tree = _use_comma_start ? ngt.find(COMMA)->tree() : ngt.tree();
            for(auto r = tree.next_char_begin(); r != tree.next_char_end(); ++r) {
                if ((first == Prefix_Tree::EMPTY) || (r->symbol() == first)) {
                    _next_char(*r);
                }
            }
        }
        else {
            _next_word();
        }

        if (_use_comma_start) {
            _words.pop_back();
        }
        _clear_fixed.clear();
    }
private:
    using It_Pair = std::pair<const Prefix_Tree *, const Prefix_Tree *>;
    using Tree_Pos = std::pair<It_Pair, score_t>;

    class Set {
    public:
        Set(const Prefix_Tree &tree, score_t other): _tree(tree), _other(other) {
        }
        const Prefix_Tree &tree() const {
            return _tree;
        }
        score_t other() const {
            return _other;
        }
        Tree_Pos to_tree_pos() const {
            return {{_tree.next_char_begin(), _tree.next_char_end()}, _other};
        }
    private:
        const Prefix_Tree &_tree;
        score_t _other;
    };
    bool push_clear(char ch) {
        if ((_clear.size() < _clear_fixed.size()) && (_clear_fixed[_clear.size()] != Prefix_Tree::EMPTY) && (ch != _clear_fixed[_clear.size()])) {
            return false;
        }
        if (_matcher.push(_clear, _cipher, ch)) {
            _clear.push_back(ch);
            return true;
        }
        else {
            return false;
        }
    }
    void pop_clear() {
        char ch = _clear.back();
        _clear.pop_back();
        _matcher.pop(_clear, _cipher, ch);
    }
    score_t acceptable(score_t word_score) const {
        score_t current = _score + _score_category + std::max(_score_other, word_score);
        score_t base = _result.low_score_limit() * static_cast<score_t>(_result.low_score_area());
        if (_clear.size() <= _result.low_score_area()) {
            return (current <= base);
        }
        else {
            score_t tail = _result.high_score_limit() * static_cast<score_t>(_clear.size() - _result.low_score_area());
            return (current <= base + tail);
        }
    }

    word_id word_tree_rev(size_t n) const {
        return _dict.word_id_map().category(_words[_words.size() - 1 - n].id());
    }

    score_t find_word_score(const Prefix_Tree &tree) {
        return tree.score();
    }
    template <class ..._Sets>
    score_t find_word_score(const Prefix_Tree &tree, const Set &s, const _Sets &...tree_n) {
        if (s.tree().is_word()) {
            return s.tree().score();
        }
        else {
            _score_other = std::max(_score_other, s.other());
            return find_word_score(tree, tree_n...);
        }
    }

    score_t calc_set_min_score(const Prefix_Tree &tree) {
        return tree.min_score();
    }
    template <class ..._Sets>
    score_t calc_set_min_score(const Prefix_Tree &tree, const Set &s, const _Sets &...tree_n) {
        if (!s.tree().empty()) {
            return s.tree().min_score();
        }
        else {
            return calc_set_min_score(tree, tree_n...);
        }
    }

    class Best_Scores {
    public:
        Best_Scores(const Word_Ngram_Tree &t):
        proper((t.proper_hits() > 0), t.proper_score()),
        numeric((t.numeric_hits() > 0), t.numeric_score()),
        comma((t.comma_hits() > 0), t.comma_score()) {
        }
        Best_Scores(const Best_Scores &that, const Word_Ngram_Tree &t):
        proper(that.proper.first || (t.proper_hits() > 0), that.proper.first ? that.proper.second : std::max(that.proper.second, t.proper_score())),
        numeric(that.numeric.first || (t.numeric_hits() > 0), that.numeric.first ? that.numeric.second : std::max(that.numeric.second, t.numeric_score())),
        comma(that.comma.first || (t.comma_hits() > 0), that.comma.first ? that.comma.second : std::max(that.comma.second, t.comma_score())) {
        }
        std::pair<bool, score_t> proper, numeric, comma;
    };

    template <size_t _K, size_t _N, class ..._Sets>
    Best_Scores next_char_tree(const Prefix_Tree &tree, const Word_Ngram_Tree &ngram_tree, const _Sets &...tree_n) {
        if constexpr(_K < _N) {
            const Word_Ngram_Tree *ngt = (_words.size() > _K) ? ngram_tree.find(word_tree_rev(_K)) : nullptr;
            if (ngt != nullptr) {
                Set s(ngt->tree(), ngt->other());
                auto p = next_char_tree<_K + 1, _N>(tree, *ngt, s, tree_n...);
                return Best_Scores(p, ngram_tree);
            }
            else {
                score_t w = calc_set_min_score(tree, tree_n...);
                if (acceptable(w)) {
                    next_char(tree, tree_n...);
                }
                return Best_Scores(ngram_tree);
            }
        }
        else {
            score_t w = calc_set_min_score(tree, tree_n...);
            if (acceptable(w)) {
                next_char(tree, tree_n...);
            }
            return Best_Scores(ngram_tree);
        }
    }
    void _next_word() {
        score_t save_other = _score_other;
        score_t save_category = _score_category;

        const Word_Ngram_Tree &nt = _dict.word_ngram_tree();
        const Word_Ngram_Tree &pt = _dict.proper_tree();
        const Word_Ngram_Tree &ut = _dict.numeric_tree();
        _score_other = 0;

        _score_category = 0;
        Best_Scores s = next_char_tree<0, 5>(nt.tree(), nt);

        _score_category = s.proper.second;
        next_char_tree<0, 1>(pt.tree(), pt);

        _score_category = s.numeric.second;
        next_char_tree<0, 1>(ut.tree(), ut);

        if (_use_comma_inside || (_clear.size() + 1 >= _cipher.size())) {
            _score_category = 0;
            _score += s.comma.second;
            _words.emplace_back(COMMA, s.comma.second, 0, 0);
            next_char_tree<0, 5>(nt.tree(), nt);
            _words.pop_back();
            _score -= s.comma.second;
        }

        _score_category = save_category;
        _score_other = save_other;
    }

    template <class ..._Sets>
    void next_word(const Prefix_Tree &tree, const _Sets &...tree_n) {
        score_t save_other = _score_other;

        score_t word_score = find_word_score(tree, tree_n...);
        if (acceptable(word_score)) {
            score_t w = std::max(_score_other, word_score);
            _score += _score_category + w;
            _words.emplace_back(tree.word(), word_score, _score_category, _score_other);

            _result.test_better(_clear, _score, _matcher, _words);
            _next_word();

            _words.pop_back();
            _score -= _score_category + w;
        }
        _score_other = save_other;
    }

    template <size_t _N, class ..._Sets>
    void _test(const std::array<Tree_Pos, _N> &, char, const Prefix_Tree &tree, const _Sets &...tree_n) {
        _matcher.test(_clear, _cipher, [&](){
            next_char(tree, tree_n...);
        });
    }

    template <size_t _K, size_t _N, class ..._Sets>
    void next_char_fixed(const std::array<Tree_Pos, _N> &it, char symbol, const Prefix_Tree &tree, const _Sets &...tree_n) {
        if constexpr(_K < _N) {
            _next_char_fixed<_K, _N>(it, symbol, tree, tree_n...);
        }
        else {
            score_t word_score = calc_set_min_score(tree, tree_n...);
            if (acceptable(word_score)) {
                _test<_N>(it, symbol, tree, tree_n...);
            }
        }
    }

    template <size_t _K, size_t _N, class ..._Sets>
    void _next_char_fixed(const std::array<Tree_Pos, _N> &it, char symbol, const Prefix_Tree &tree, const Set &s, const _Sets &...tree_n) {
        const It_Pair &w = it[_K].first;
        if ((w.first != w.second) && (w.first->symbol() == symbol)) {
            Set ns(*w.first, s.other());
            next_char_fixed<_K + 1, _N>(it, symbol, tree, tree_n..., ns);
        }
        else {
            score_t save = _score_other;
            _score_other = std::max(_score_other, s.other());
            next_char_fixed<_K + 1, _N>(it, symbol, tree, tree_n...);
            _score_other = save;
        }
    }

    template <class ..._Sets>
    auto create_subtree_iters(const _Sets &...tree_n) const {
        std::array<Tree_Pos, sizeof...(_Sets)> arr = {tree_n.to_tree_pos()...};
        return arr;
    }
    template <class _Arr>
    void move_iters(_Arr &arr, char ch) const {
        for(auto &p: arr) {
            It_Pair &w = p.first;
            while ((w.first != w.second) && (w.first->symbol() < ch)) {
                ++w.first;
            }
        }
    }

    template <class ..._Sets>
    void _next_char(const Prefix_Tree &tree, const _Sets &...tree_n) {
        if (_clear.size() < _cipher.size()) {
            auto it = create_subtree_iters(tree_n...);
            for(auto r = tree.next_char_begin(); r != tree.next_char_end(); ++r) {
                if (push_clear(r->symbol())) {
                    move_iters(it, r->symbol());
                    next_char_fixed<0, sizeof...(_Sets)>(it, r->symbol(), *r, tree_n...);
                    pop_clear();
                }
            }
        }
        else if (tree.is_root()) {
            if (!_words.empty() && (_words.back().id() == COMMA)) {
                _result.test_best(_clear, _score, _matcher, _words);
            }
        }
    }

    template <class ..._Sets>
    void next_char(const Prefix_Tree &tree, const _Sets &...tree_n) {
        if (tree.is_word()) {
            next_word(tree, tree_n...);
        }
        else if ((_filler != Prefix_Tree::EMPTY) && (_clear.size() % 2 == 1) && push_clear('x')) { // try insert x
            char last = _clear[_clear.size() - 2];
            if (_clear.size() >= _cipher.size()) {
                _matcher.test(_clear, _cipher, [&](){
                    next_char(tree, tree_n...);
                });
            }
            else if (push_clear(last)) {
                const Prefix_Tree *t = tree.find_sub_tree(last);
                if (t != nullptr) {
                    auto it = create_subtree_iters(tree_n...);
                    move_iters(it, last);
                    next_char_fixed<0, sizeof...(_Sets)>(it, last, *t, tree_n...);
                }
                pop_clear();
            }
            pop_clear();
        }
        _next_char(tree, tree_n...);
    }

    _Matcher            _matcher;

    const Dictionary    &_dict;
    Result              &_result;
    std::string         _clear_fixed;
    std::string         _clear;
    score_t             _score;
    score_t             _score_category;
    score_t             _score_other;

    Word_List           _words;
    std::string         _cipher;
    bool                _odd_mode;
    bool                _use_comma_start;
    bool                _use_comma_inside;
    char                _filler;
};

class Queue {
public:
    Queue(size_t depth, Result &result): _result(result), _pos(0), _letters("taioswcbphfmdrelngyukvqxz") {
        //std::reverse(std::begin(_letters), std::end(_letters));
        add(depth, "");
    }
    std::string pop(size_t n) {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_pos < _list.size()) {
            _result.print_state(n, _list[_pos], _pos, _list.size());
        }
        return (_pos < _list.size()) ? _list[_pos++] : std::string();
    }
private:
    void add(size_t n, const std::string &s) {
        if (n > 0) {
            for(char ch: _letters) {
                add(n - 1, s + std::string(1, ch));
            }
        }
        else {
            _list.push_back(s);
        }
    }
    Result &_result;
    size_t      _pos;
    std::string _letters;
    std::vector<std::string>    _list;
    std::mutex          _mtx;
};

bool option(char ch, std::string &s) {
    if ((s.size() >= 2) && (s[0] =='-') && (s[1] == ch)) {
        s = s.substr(2);
        return true;
    }
    else {
        return false;
    }
}

size_t str_to_size(const std::string &s) {
    return static_cast<size_t>(std::stoi(s));
}

class Task {
public:
    Task(size_t low_score_area, score_t low_score_limit,  score_t high_score_limit,
    size_t iterations, size_t threads, size_t queue_size,
    size_t matrix_creation_point, bool odd_mode, bool use_comma_start, bool use_comma_inside, char filler,
    size_t print_solutions,
    const std::string &cipher, const std::string &clear_fixed):
    _low_score_area(low_score_area), _low_score_limit(low_score_limit), _high_score_limit(high_score_limit),
    _iterations(iterations), _threads(threads), _queue_size(queue_size),
    _matrix_creation_point(matrix_creation_point), _odd_mode(odd_mode),
    _use_comma_start(use_comma_start), _use_comma_inside(use_comma_inside), _filler(filler),
    _print_solutions(print_solutions),
    _cipher(cipher), _clear_fixed(clear_fixed) {
        for(char &ch: _clear_fixed) {
            if (ch == '_') {
                ch = Prefix_Tree::EMPTY;
            }
        }
    }

    void execute(const std::string &type, const Dictionary &dict) const {
        std::cout << std::endl;
        if (_threads > 0) {
            std::cout << "Threads: " << _threads << std::endl;
        }
        std::cout << "Ciphertext: " << _cipher << "(" << _cipher.size() << ")" << std::endl;
        if (!_clear_fixed.empty()) {
            std::cout << "Cleartext beginning: " << _clear_fixed << "(" << _clear_fixed.size() << ")" << std::endl;
        }
        std::cout << "Low score area: " << _low_score_area << std::endl;
        std::cout << "Low score limit per char: " << score_to_str(_low_score_limit) << std::endl;
        std::cout << "High score limit per char: " << score_to_str(_high_score_limit) << std::endl;
        std::cout << "Matrix creation point: " << _matrix_creation_point << std::endl;
        std::cout << "Start comma: " << (_use_comma_start ? "yes" : "no") << std::endl;
        std::cout << "Inside comma: " << (_use_comma_inside ? "yes" : "no") << std::endl;
        std::cout << "Odd mode: " << (_odd_mode ? "yes" : "no") << std::endl;
        std::cout << "Print detalization: " << _print_solutions << std::endl;
        std::cout << std::endl;

        Result result(dict.word_id_map(), _low_score_area, _low_score_limit, _high_score_limit, _print_solutions);

        if (type == "playfair") {
            search(playfair::Playfair(_matrix_creation_point), dict, result);
        }
        else if (type == "chaotic") {
            search(chaotic::Chaotic(), dict, result);
        }
        else if (type == "simple") {
            search(simple::Simple(), dict, result);
        }
        else if (type == "pelling") {
            search(simple::Pelling(5), dict, result);
        }
        else if (type == "bigram") {
            search(simple::Bigram(), dict, result);
        }
        else {
            std::terminate();
        }


        result.print_result_lists(true);
        std::cout << std::endl;
        std::cout << "Task finished" << std::endl;
        std::cout << std::endl;
    }
private:
    template <class _Search>
    void search_threaded(_Search &s, Result &result) const {
        Queue queue(_queue_size, result);
        auto func = [this, s, &queue](size_t n) mutable {
            std::string w = queue.pop(n);
            while (!w.empty()) {
                s(_clear_fixed + w);
                w = queue.pop(n);
            };
        };
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < _threads; ++i) {
            futures.emplace_back(std::async(std::launch::async, func, i));
        }
        for (auto &f: futures) {
            f.get();
        }
    }

    template <class _Matcher>
    void search(const _Matcher &matcher, const Dictionary &dict, Result &result) const {
        Search<_Matcher> s(matcher, dict, result, _cipher, _odd_mode, _use_comma_start, _use_comma_inside, _filler);

        for(size_t i = 0; i < _iterations; ++i) {
            auto start = std::chrono::steady_clock::now();
            if (_threads > 0) {
                search_threaded(s, result);
            }
            else {
                s(_clear_fixed);
            }
            auto v = std::chrono::steady_clock::now();
            auto d = std::chrono::duration_cast<std::chrono::milliseconds>(v - start);
            std::cout << "i" << i << ": " << d.count() << std::endl;
        }
    }

    size_t _low_score_area;
    score_t _low_score_limit;
    score_t _high_score_limit;
    size_t _iterations;
    size_t _threads;
    size_t _queue_size;
    size_t _matrix_creation_point;
    bool _odd_mode;
    bool _use_comma_start;
    bool _use_comma_inside;
    char _filler;
    size_t _print_solutions;
    std::string _cipher;
    std::string _clear_fixed;
};

int main(int argc, char* args[]) {
    std::vector<std::string> stat_files;
    std::vector<std::string> nprop_files;
    std::vector<std::string> prop_files;
    std::vector<std::string> numeric_files;
    std::string type;
    char filler = Prefix_Tree::EMPTY;
    size_t max_word_count = 100000;
    size_t low_score_area = 16;
    score_t low_score_limit = 0;
    score_t high_score_limit = 0;
    size_t iterations = 1;
    size_t threads = 0;
    size_t queue_size = 2;
    size_t matrix_creation_point = 20;
    std::string cipher;
    std::string clear_fixed;
    std::vector<Task> task_list;
    bool odd_mode = false;
    bool use_comma_start = false;
    bool use_comma_inside = false;
    size_t print_solutions = 1; // only solutions which update top list

    for(int p = 1; p < argc; ++p) {
        std::string w = args[p];
        if (option('s', w)) {
            stat_files.push_back(w);
        }
        else if (option('x', w)) {
            type = w;
        }
        else if (option('n', w)) {
            nprop_files.push_back(w);
        }
        else if (option('p', w)) {
            prop_files.push_back(w);
        }
        else if (option('u', w)) {
            numeric_files.push_back(w);
        }
        else if (option('a', w)) {
            low_score_area = str_to_size(w);
        }
        else if (option('l', w)) {
            low_score_limit = static_cast<score_t>(std::stod(w) * WORD_SCORE_UNIT);
        }
        else if (option('h', w)) {
            high_score_limit = static_cast<score_t>(std::stod(w) * WORD_SCORE_UNIT);
        }
        else if (option('i', w)) {
            iterations = str_to_size(w);
        }
        else if (option('t', w)) {
            threads = str_to_size(w);
        }
        else if (option('q', w)) {
            queue_size = str_to_size(w);
        }
        else if (option('w', w)) {
            max_word_count = str_to_size(w);
        }
        else if (option('m', w)) {
            matrix_creation_point = str_to_size(w);
        }
        else if (option('c', w)) {
            clear_fixed = to_lower(w);
        }
        else if (option('f', w)) {
            filler = w[0];
        }
        else if (option('O', w)) {
            odd_mode = (w != "off");
        }
        else if (option('S', w)) {
            use_comma_start = (w != "off");
        }
        else if (option('C', w)) {
            use_comma_inside = (w != "off");
        }
        else if (option('P', w)) {
            print_solutions = str_to_size(w);
        }
        else {
            cipher = to_lower(w);
            task_list.emplace_back(low_score_area, low_score_limit, high_score_limit, iterations, threads, queue_size, matrix_creation_point, odd_mode, use_comma_start, use_comma_inside, filler, print_solutions, cipher, clear_fixed);
        }
    }
    std::cout << "Cipher type: " << type << std::endl;
    std::cout << "Tasks: " << task_list.size() << std::endl;
    std::cout << "Score unit: " << WORD_SCORE_UNIT << std::endl;
    std::cout << "Max word count: " << max_word_count << std::endl;

    auto execute_tasks = [&](auto conv) {
        Dictionary dict(conv, stat_files, nprop_files, prop_files, numeric_files, max_word_count);

        for(const Task &task: task_list) {
            task.execute(type, dict);
        }
    };

    if (type == "playfair") {
        execute_tasks(Converter_JI());
    }
    else {
        execute_tasks(Common_Converter());
    }

    system("pause");
    return 0;
}
