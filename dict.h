/*
 * Copyright (c) Konstantin Hamidullin. All rights reserved.
 */

#define Map_Type std::unordered_map

using score_t = int64_t;
using small_score_t = int16_t;
using hits_t = uint32_t;

using word_id = uint32_t;
constexpr word_id NONE = 0;
constexpr word_id PROPER = 1;
constexpr word_id COMMA = 2;
constexpr word_id NUMERIC = 3;

constexpr small_score_t WORD_SCORE_UNIT = 100;
constexpr small_score_t INF_SCORE = std::numeric_limits<small_score_t>::max();
constexpr size_t MAX_CURRENT_PRINT = 20;
constexpr size_t MAX_FINAL_PRINT = 5000;
constexpr double ANOTHER_WORD_HITS = 0.5;

small_score_t calc_score(size_t hits, size_t max) {
    if (max == 0) {
        return 0;
    }
    double r = (hits == 0) ? ANOTHER_WORD_HITS : static_cast<double>(hits);
    r /= static_cast<double>(max);
    r = -log2(r);
    return static_cast<small_score_t>(r * static_cast<double>(WORD_SCORE_UNIT));
}

std::string score_to_str(score_t s) {
    /*double w = static_cast<double>(s) / static_cast<double>(WORD_SCORE_UNIT);
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << w;
    return stream.str();*/
    return std::to_string(s);
}

char to_lower(char ch) {
    return static_cast<char>(std::tolower(ch));
}

std::string to_lower(std::string s) {
    for (char &ch: s) {
        ch = to_lower(ch);
    }
    return s;
}

bool is_numeric(const std::string &s) {
    if ((s.size() > 0) && (s.front() >= '0') && (s.front() <= '9')) {
        return true;
    }
    return false;
}

class Word {
public:
    Word(word_id id, score_t score, score_t category, score_t other):
    _id(id), _score(score), _category(category), _other(other) {
    }
    word_id id() const {
        return _id;
    }
    score_t score() const {
        return _score;
    }
    score_t category() const {
        return _category;
    }
    score_t other() const {
        return _other;
    }
    bool operator<(const Word &that) const {
        if (_id < that._id) return true;
        if (_id > that._id) return false;

        if (_score < that._score) return true;
        if (_score > that._score) return false;

        if (_category < that._category) return true;
        if (_category > that._category) return false;

        return (_other < that._other);
    }
private:
    word_id     _id;
    score_t     _score;
    score_t     _category;
    score_t     _other;
};

using Word_List = std::vector<Word>;
using Ticks = std::chrono::time_point<std::chrono::steady_clock>;
using Word_Id_List = std::vector<std::pair<std::string, word_id>>;

using Word_Frequency_List = std::vector<std::pair<word_id, hits_t>>;
using Word_Frequency_Map = std::map<word_id, hits_t>;

template <class _T, class _C>
std::vector<std::pair<_T, hits_t>> sort_freq(const _C &v) {
    std::vector<std::pair<_T, hits_t>> result;
    result.reserve(v.size());
    for (const auto &w: v) {
        result.push_back(w);
    }
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });
    return result;
}

class Word_Id_Map {
public:
    class Bimap {
    public:
        Bimap() {
            _id_to_word.reserve(60000);
        }
        size_t size() const {
            return _id_to_word.size();
        }
        const std::string &word_by_id(word_id id) const {
            return _id_to_word[id];
        }
        word_id id_by_word(const std::string &w) const {
            auto it = _word_to_id.find(w);
            assert(it != _word_to_id.end());
            return it->second;
        }
        word_id add(const std::string &s) {
            auto it = _word_to_id.find(s);
            if (it == _word_to_id.end()) {
                word_id n = static_cast<word_id>(_id_to_word.size());
                _word_to_id[s] = n;
                _id_to_word.push_back(s);
                return n;
            }
            else {
                return it->second;
            }
        }
    private:
        std::vector<std::string> _id_to_word;
        Map_Type<std::string, word_id> _word_to_id;
    };
    Word_Id_Map(): _proper_start(500000), _numeric_start(900000) {
        word_id none = _bimap_nproper.add("*");
        word_id proper = _bimap_nproper.add("<proper>");
        word_id comma = _bimap_nproper.add("$");
        word_id numeric = _bimap_nproper.add("{numeric}");
        assert(none == NONE);
        assert(proper == PROPER);
        assert(comma == COMMA);
        assert(numeric == NUMERIC);
    }
    std::set<std::string> &nproper() {
        return _nproper;
    }
    std::set<std::string> &proper() {
        return _proper;
    }
    std::set<std::string> &numeric() {
        return _numeric;
    }
    word_id add_proper(const std::string &s) {
        return _proper_start + _bimap_proper.add(s);
    }
    word_id add_numeric(const std::string &s) {
        return _numeric_start + _bimap_numeric.add(s);
    }
    word_id add(const std::string &s) {
        if (s == "$") {
            return COMMA;
        }
        else if (is_numeric(s)) {
            return NUMERIC;
        }
        else if (_numeric.find(s) != _numeric.end()) {
            return _numeric_start + _bimap_numeric.add(s);
        }
        else if (_proper.find(s) != _proper.end()) {
            return _proper_start + _bimap_proper.add(s);
        }
        else if (_nproper.find(s) == _nproper.end()) {
            return NONE;
        }
        else {
            return _bimap_nproper.add(s);
        }
    }
    word_id category(word_id id) const {
        if (id >= _numeric_start) {
            return NUMERIC;
        }
        else if (id >= _proper_start) {
            return PROPER;
        }
        else {
            return id;
        }
    }
    std::string word_by_id(word_id id) const {
        if (id == NONE) {
            return "*";
        }
        else if (id >= _numeric_start) {
            return (std::string("{") + _bimap_numeric.word_by_id(id - _numeric_start) + std::string("}"));
        }
        else if (id >= _proper_start) {
            return (std::string("<") + _bimap_proper.word_by_id(id - _proper_start) + std::string(">"));
        }
        else {
            return _bimap_nproper.word_by_id(id);
        }
    }
    word_id id_by_word(const std::string &w) const {
        return _bimap_nproper.id_by_word(w);
    }
private:
    std::set<std::string> _nproper, _proper, _numeric;
    Bimap       _bimap_nproper;
    Bimap       _bimap_proper;
    Bimap       _bimap_numeric;
    word_id     _proper_start;
    word_id     _numeric_start;
};

void print_list(const Word_Id_Map &word_id_map, const Word_Frequency_List &list, size_t max) {
    size_t i = 0;
    for (const auto &w: list) {
        if (++i > max) {
            break;
        }
        std::cout << word_id_map.word_by_id(w.first) << " " << w.second << std::endl;
    }
    std::cout << std::endl;
}

class Prefix_Tree {
public:
    static constexpr char EMPTY = ' ';
    Prefix_Tree(): Prefix_Tree(EMPTY) {
    }
    Prefix_Tree(char symbol): _word(NONE), _size(0), _symbol(symbol & ((1 << 7) - 1)),
    _hits(0), _next_char(nullptr) {
    }
    Prefix_Tree(const Prefix_Tree &that) = delete;
    Prefix_Tree(Prefix_Tree &&that) noexcept: _word(that._word), _size(that._size), _symbol(that._symbol),
    _hits(that._hits), _next_char(that._next_char) {
        that._next_char = nullptr;
    }
    Prefix_Tree &operator=(const Prefix_Tree &that) = delete;
    Prefix_Tree &operator=(Prefix_Tree &&that) noexcept {
        _word = that._word;
        _size = that._size;
        _symbol = that._symbol;
        _hits = that._hits;
        _next_char = that._next_char;
        that._next_char = nullptr;
        return *this;
    }

    ~Prefix_Tree() {
        delete [] _next_char;
    }

    bool is_root() const {
        return (_symbol == EMPTY);
    }
    bool is_word() const {
        return (_word != NONE);
    }
    bool empty() const {
        return !is_word() && (_size == 0);
    }
    word_id word() const {
        return _word;
    }
    char symbol() const {
        return _symbol;
    }
    hits_t hits() const {
        return _hits;
    }
    score_t score() const {
        return _primary._score;
    }
    score_t min_score() const {
        return _primary._min_score;
    }
    const Prefix_Tree *find_sub_tree(char ch) const {
        for(int32_t i = 0; i < _size; ++i) {
            if (_next_char[i]._symbol == ch) {
                return &(_next_char[i]);
            }
        }
        return nullptr;
    }
    const Prefix_Tree *next_char_begin() const {
        return &(_next_char[0]);
    }
    const Prefix_Tree *next_char_end() const {
        return &(_next_char[_size]);
    }

    void create_list(Word_Frequency_List &list) const {
        if (is_word()) {
            list.emplace_back(_word, _hits);
        }
        for(int32_t i = 0; i < _size; ++i) {
            _next_char[i].create_list(list);
        }
    }
    Word_Frequency_List create_list() const {
        Word_Frequency_List list;
        create_list(list);
        return list;
    }
    void add_hits(const std::string_view &s, word_id id, hits_t h) {
        if (s.empty()) {
            _word = id & ((1 << 20) - 1);
            _hits += h;
        }
        else {
            char ch = s[0];
            for(int32_t i = 0; i < _size; ++i) {
                if (_next_char[i]._symbol == ch) {
                    _next_char[i].add_hits(s.substr(1), id, h);
                    return;
                }
            }
            Prefix_Tree *w = new Prefix_Tree[_size + 1];
            //_next_char.emplace_back(ch, std::move(t));
            for(int32_t i = 0; i < _size; ++i) {
                w[i] = std::move(_next_char[i]);
            }
            w[_size] = Prefix_Tree(ch);
            w[_size].add_hits(s.substr(1), id, h);

            delete [] _next_char;
            _next_char = w;
            _size++;
        }
    }
    std::pair<score_t, size_t> calc_scores(size_t l, size_t max_hits) {
        std::pair<score_t, size_t> result = {0, 0};
        if (is_word()) {
            size_t hits = _hits;
            _primary._score = calc_score(hits, max_hits);
            result.first += static_cast<score_t>(hits) * _primary._score;
            result.second += hits * l;
        }
        else {
            _primary._score = INF_SCORE;
        }
        _primary._min_score = _primary._score;
        for(int32_t i = 0; i < _size; ++i) {
            auto w = _next_char[i].calc_scores(l + 1, max_hits);
            result.first += w.first;
            result.second += w.second;
            _primary._min_score = std::min(_primary._min_score, _next_char[i]._primary._min_score);
        }
        sort_chars();
        return result;
    }
    void adjust_scores(small_score_t add, small_score_t add_delta, small_score_t nom, small_score_t denom, small_score_t min) {
        if (is_word()) {
            _primary._score = std::max(static_cast<small_score_t>((_primary._score + add) * nom / denom), min);
        }
        _primary._min_score = _primary._score;
        for(int32_t i = 0; i < _size; ++i) {
            _next_char[i].adjust_scores(static_cast<small_score_t>(add + add_delta), add_delta, nom, denom, min);
            _primary._min_score = std::min(_primary._min_score, _next_char[i]._primary._min_score);
        }
        sort_chars();
    }

    std::pair<score_t, std::pair<score_t, size_t>> calc_scores(bool use_max) {
        hits_t mh = empty() ? 0 : (use_max ? max_hits() : total_hits());
        return {calc_score(0, mh), calc_scores(0, mh)};
    }

    void print(size_t level) const {
        std::cout << std::string(level * 2, ' ') << _symbol << " <id=" << _word << "> s=" << _primary._score << " ms=" << _primary._min_score << " h=" << _hits << std::endl;
        for(int32_t i = 0; i < _size; ++i) {
            _next_char[i].print(level + 1);
        }
    }
    const Prefix_Tree *find(const std::string_view &s) const {
        if (s.empty()) {
            return this;
        }
        else {
            char ch = s[0];
            for(int32_t i = 0; i < _size; ++i) {
                if (_next_char[i]._symbol == ch) {
                    return _next_char[i].find(s.substr(1));
                }
            }
            return nullptr;
        }
    }
    hits_t max_hits() const {
        if (is_word()) {
            hits_t result = _hits;
            for(int32_t i = 0; i < _size; ++i) {
                result = std::max(result, _next_char[i].max_hits());
            }
            return result;
        }
        else {
            hits_t result = _next_char[0].max_hits();
            int32_t n = 0;
            while (++n < _size) {
                result = std::max(result, _next_char[n].max_hits());
            }
            return result;
        }
    }
    hits_t total_hits() const {
        hits_t result = 0;
        if (is_word()) {
            result += _hits;
        }
        for(int32_t i = 0; i < _size; ++i) {
            result += _next_char[i].total_hits();
        }
        return result;
    }
private:
    void sort_chars() {
        std::sort(&(_next_char[0]), &(_next_char[_size]), [](const auto& a, const auto &b){
            return a._symbol < b._symbol;
        });
    }
    struct Score_Values {
        small_score_t     _score;
        small_score_t     _min_score;
    };

    uint32_t    _word:20;
    uint32_t    _size:5;
    uint32_t    _symbol:7;
    union {
        hits_t          _hits;
        Score_Values    _primary;
    };
    Prefix_Tree       *_next_char;
};

class Word_Ngram_Tree_Map;

class Word_Ngram_Tree {
public:
    Word_Ngram_Tree();
    Word_Ngram_Tree(const Word_Ngram_Tree &) = delete;
    Word_Ngram_Tree(const Word_Ngram_Tree &&) = delete;
    Word_Ngram_Tree &operator=(const Word_Ngram_Tree &&) = delete;
    ~Word_Ngram_Tree();
    void add(const Word_Id_Map &word_id_map, const Word_Id_List &words, hits_t h, bool tail_original);
    const Word_Ngram_Tree *find() const {
        return this;
    }
    template <class... _Id>
    const Word_Ngram_Tree *find(word_id id, _Id... ids) const {
        const Word_Ngram_Tree *w =_find(id);
        return (w == nullptr) ? nullptr : w->find(ids...);
    }
    std::pair<score_t, size_t> calc_scores(bool use_max);
    void adjust_scores(small_score_t add, small_score_t add_delta, small_score_t nom, small_score_t denom, small_score_t min = std::numeric_limits<small_score_t>::min());
    size_t total() const {
        return _total;
    }
    score_t other() const {
        return _other;
    }
    score_t proper_score() const {
        return _proper_score;
    }
    score_t numeric_score() const {
        return _numeric_score;
    }
    score_t comma_score() const {
        return _comma_score;
    }
    hits_t proper_hits() const {
        return _proper_hits;
    }
    hits_t numeric_hits() const {
        return _numeric_hits;
    }
    hits_t comma_hits() const {
        return _comma_hits;
    }
    const Prefix_Tree &tree() const {
        return _tree;
    }
    Prefix_Tree &tree() {
        return _tree;
    }
    Word_Frequency_List create_list() const {
        Word_Frequency_List list = _tree.create_list();
        if (_proper_hits > 0) {
            list.emplace_back(PROPER, _proper_hits);
        }
        if (_numeric_hits > 0) {
            list.emplace_back(NUMERIC, _numeric_hits);
        }
        if (_comma_hits > 0) {
            list.emplace_back(COMMA, _comma_hits);
        }
        return list;
    }
    void print_frequencies(const Word_Id_Map &word_id_map, size_t max) const {
        auto list = sort_freq<word_id>(create_list());
        std::cout << "<total> " << _total << std::endl;
        print_list(word_id_map, list, max);
    }
private:
    void _add(const Word_Id_Map &word_id_map, const Word_Id_List &words, size_t n, hits_t h, bool tail_original);
    const Word_Ngram_Tree *_find(word_id id) const;
    Word_Ngram_Tree_Map *_next;
    Prefix_Tree     _tree;
    size_t          _total;
    hits_t          _proper_hits;
    hits_t          _numeric_hits;
    hits_t          _comma_hits;
    small_score_t   _proper_score;
    small_score_t   _numeric_score;
    small_score_t   _comma_score;
    small_score_t   _other;
};

class Word_Ngram_Tree_Map: public Map_Type<word_id, Word_Ngram_Tree> {
};

Word_Ngram_Tree::Word_Ngram_Tree(): _next(nullptr),
_tree(), _total(0), _proper_hits(0), _numeric_hits(0), _comma_hits(0),
_proper_score(0), _numeric_score(0), _comma_score(0), _other(0) {
}

Word_Ngram_Tree::~Word_Ngram_Tree() {
    delete _next;
}

std::pair<score_t, size_t> Word_Ngram_Tree::calc_scores(bool use_max) {
    size_t mh = _tree.empty() ? 0 : (use_max ? _tree.max_hits() : _total);
    mh = std::max(mh, static_cast<size_t>(_proper_hits));
    mh = std::max(mh, static_cast<size_t>(_numeric_hits));
    mh = std::max(mh, static_cast<size_t>(_comma_hits));
    _other = calc_score(0, mh);
    _proper_score = calc_score(_proper_hits, mh);
    _numeric_score = calc_score(_numeric_hits, mh);
    _comma_score = calc_score(_comma_hits, mh);
    auto result = _tree.calc_scores(0, mh);
    if (_next != nullptr) {
        for (auto &t: *_next) {
            auto q = t.second.calc_scores(use_max);
            result.first += q.first;
            result.second += q.second;
        }
    }
    return result;
}

void Word_Ngram_Tree::adjust_scores(small_score_t add, small_score_t add_delta, small_score_t nom, small_score_t denom, small_score_t min) {
    _tree.adjust_scores(add, add_delta, nom, denom, min);
    if (_next != nullptr) {
        for (auto &t: *_next) {
            t.second.adjust_scores(add, add_delta, nom, denom, min);
        }
    }
}

void Word_Ngram_Tree::add(const Word_Id_Map &word_id_map, const Word_Id_List &words, hits_t h, bool tail_original) {
    if (words.size() == 0) {
        return;
    }
    _add(word_id_map, words, words.size() - 1, h, tail_original);
}

void Word_Ngram_Tree::_add(const Word_Id_Map &word_id_map, const Word_Id_List &words, size_t n, hits_t h, bool tail_original) {
    if (n > 0) {
        if (words[n - 1].second != NONE) {
            word_id id = word_id_map.category(words[n - 1].second);
            if (_next == nullptr) {
                _next = new Word_Ngram_Tree_Map();
            }
            (*_next)[id]._add(word_id_map, words, n - 1, h, tail_original);
        }
    }
    else if (words.back().second != NONE) {
        _total += h;
        word_id id = tail_original ? words.back().second : word_id_map.category(words.back().second);
        if (id == PROPER) {
            _proper_hits += h;
        }
        else if (id == NUMERIC) {
            _numeric_hits += h;
        }
        else if (id == COMMA) {
            _comma_hits += h;
        }
        else {
            _tree.add_hits(words.back().first, id, h);
        }
    }
}

const Word_Ngram_Tree *Word_Ngram_Tree::_find(word_id id) const {
    if (_next == nullptr) {
        return nullptr;
    }
    auto it = _next->find(id);
    return (it == _next->end()) ? nullptr : &(it->second);
}

bool test_word(const std::string &s) {
    for (auto ch: s) {
        bool l = (ch >= 'a') && (ch <= 'z');
        bool u = (ch >= 'A') && (ch <= 'Z');
        if (!l && !u) {
            return false;
        }
    }
    return true;
}

class Common_Converter {
public:
    std::string operator()(std::string s) const {
        return s;
    }
};


class Converter_JI {
public:
    std::string operator()(std::string s) const {
        for (auto &ch: s) {
            if (ch == 'j') {
                ch = 'i';
            }
            if (ch == 'J') {
                ch = 'I';
            }
        }
        return s;
    }
};

class Dictionary {
public:
    template <class _Conv>
    void load_words(const std::string &filename, const _Conv &conv, std::set<std::string> &words) {
        std::ifstream file(filename);
        std::string s;
        while (getline(file, s)) {
            if (test_word(s)) {
                s = conv(to_lower(s));
                words.insert(s);
            }
        }
    }
    template <class _Conv>
    void load_proper(const std::string &filename, const _Conv &conv, std::set<std::string> &words) {
        std::ifstream file(filename);
        std::string s;
        while (getline(file, s)) {
            size_t p = s.find('\t');
            assert((p != std::string::npos) && (p != 0));
            if (s[p + 1] == 'N') {
                s = s.substr(0, p);

                size_t p = s.find_first_of(' ');
                if (p != std::string::npos) {
                    s = s.substr(0, p);
                }

                if (s.size() >= 4) {
                    for (size_t i = 1; i < s.size(); ++i) {
                        if ((s[i] >= 'A') && (s[i] <= 'Z') && ((s[i - 1] == '-') || (s[i - 1] == '\'') || ((s[i - 1] >= 'a') && (s[i - 1] <= 'z')))) {
                            s[i] = to_lower(s[i]);
                        }
                    }
                    std::string w;
                    for (char ch: s) {
                        if ((ch != '-') && (ch != '\'')) {
                            w += ch;
                        }
                    }
                    bool good = (w[0] >= 'A') && (w[0] <= 'Z');
                    for (size_t i = 1; i < w.size(); ++i) {
                        if (!((w[i] >= 'a') && (w[i] <= 'z'))) {
                            good = false;
                        }
                    }
                    if (good && test_word(w)) {
                        w = conv(to_lower(w));
                        words.insert(w);
                    }
                }
            }
        }
    }
    void print_words() const {
    }
    template <class... _Id>
    void print_words(word_id id, _Id...ids) const {
        std::cout << word_id_map().word_by_id(id) << " ";
        print_words(ids...);
    }
    template <class... _Id>
    void print_words(const std::string &word, _Id...ids) const {
        std::cout << word << " ";
        print_words(ids...);
    }

    const Word_Ngram_Tree *find_tree(const Word_Ngram_Tree &source) const {
        return &source;
    }
    template <class... _Id>
    const Word_Ngram_Tree *find_tree(const Word_Ngram_Tree &source, word_id id, _Id...ids) const {
        const Word_Ngram_Tree *w = find_tree(source, ids...);
        return (w != nullptr) ? w->find(id) : nullptr;
    }
    template <class... _Id>
    const Word_Ngram_Tree *find_tree(const Word_Ngram_Tree &source, const std::string &word, _Id...ids) const {
        return find_tree(source, word_id_map().id_by_word(word), ids...);
    }

    template <class... _Id>
    score_t _test_next_word(const Word_Ngram_Tree &source, const std::string &next_word, score_t other, _Id...ids) const {
        const Word_Ngram_Tree *w = find_tree(source, ids...);
        if (w != nullptr) {
            const Prefix_Tree *t = w->tree().find(next_word);
            if ((t != nullptr) && t->is_word()) {
                print_words(ids...);
                std::cout << "-> " << next_word << " (" << t->score();
                /*if (other > 0) {
                    std::cout << "|" << other << "o";
                }*/
                std::cout << ")" << std::endl;
            }
            print_words(ids...);
            std::cout << "-> " << "<other> (" << w->other();
            /*if (other > 0) {
                std::cout << "|" << other << "o";
            }*/
            std::cout << ")" << std::endl;
            return std::max(w->other(), other);
        }
        return other;
    }
    void test_next_word(const Word_Ngram_Tree &source, const std::string &next_word, score_t other) const {
       _test_next_word(source, next_word, other);
    }
    template <class... _Id>
    void test_next_word(const Word_Ngram_Tree &source, const std::string &next_word, score_t other, word_id id, _Id...ids) const {
        score_t o = _test_next_word(source, next_word, other, id, ids...);
        test_next_word(source, next_word, o, ids...);
    }

    template <class... _Id>
    void test_next_word(const Word_Ngram_Tree &source, const std::string &next_word, score_t other, const std::string &word, _Id...ids) const {
        test_next_word(source, next_word, other, word_id_map().id_by_word(word), ids...);
    }

    template <class _Conv>
    Dictionary(const _Conv &conv, const std::vector<std::string> &stat_files, const std::vector<std::string> &nprop_files, const std::vector<std::string> &prop_files, const std::vector<std::string> &numeric_files, size_t max_word_count) {
        std::set<std::string> nproper, proper, numeric;
        for(const auto &fn: nprop_files) {
            std::cout << "Loading protected non-proper name file " << fn << "...";
            load_words(fn, conv, nproper);
            std::cout << " Done" << std::endl;
        }

        for(const auto &fn: prop_files) {
            std::cout << "Loading proper name file " << fn << "...";
            load_proper(fn, conv, proper);
            std::cout << " Done" << std::endl;
        }

        for(const auto &fn: numeric_files) {
            std::cout << "Loading numeral name file " << fn << "...";
            load_words(fn, conv, numeric);
            std::cout << " Done" << std::endl;
        }

        auto p = load_stats_words(stat_files, max_word_count, conv, nproper, numeric);
        _word_id_map.nproper() = std::move(p.first);
        _word_id_map.proper() = std::move(p.second);
        _word_id_map.numeric() = std::move(numeric);

        for(const auto &w: _word_id_map.nproper()) {
            assert(_word_id_map.proper().find(w) == _word_id_map.proper().end());
            _word_id_map.proper().erase(w);
        }

        for(const auto &fn: stat_files) {
            std::cout << "Loading stat file " << fn << "...";
            load_stats(fn, conv);
            std::cout << " Done" << std::endl;
        }

        // must go after load_stats
        for(const auto &w: proper) {
            word_id id = _word_id_map.add_proper(w);
            _proper_tree.add(_word_id_map, {{w, id}}, 1, true);
        }

        /*{
            const Word_Ngram_Tree *ngt = find_tree(word_ngram_tree(), "while", "in");
            if (ngt != nullptr) {
                ngt->print_frequencies(word_id_map(), 1000);
            }
        }
        {
            const Word_Ngram_Tree *ngt = find_tree(numeric_tree(), "at");
            if (ngt != nullptr) {
                ngt->print_frequencies(word_id_map(), 1000);
            }
        }*/

        /*std::pair<score_t, size_t> nprop_av_score = */_word_ngram_tree.calc_scores(false);
        /*std::pair<score_t, size_t> prop_av_score = */_proper_tree.calc_scores(false);
        _numeric_tree.calc_scores(false);

        /*
        std::pair<score_t, size_t> total_av_score = {
            nprop_av_score.first + prop_av_score.first,
            nprop_av_score.second + prop_av_score.second
        };

        std::cout << nprop_av_score.first << " " << nprop_av_score.second << std::endl;
        std::cout << "Score per char (nproper): " << (static_cast<double>(nprop_av_score.first) / static_cast<double>(nprop_av_score.second)) << std::endl;
        //small_score_t score_per_char = static_cast<small_score_t>(-nprop_av_score.first / static_cast<score_t>(nprop_av_score.second));

        std::cout << prop_av_score.first << " " << prop_av_score.second << std::endl;
        std::cout << "Score per char (proper): " << (static_cast<double>(prop_av_score.first) / static_cast<double>(prop_av_score.second)) << std::endl;
        //small_score_t proper_score_per_char = static_cast<small_score_t>(-prop_av_score.first / static_cast<score_t>(prop_av_score.second));

        std::cout << total_av_score.first << " " << total_av_score.second << std::endl;
        std::cout << "Score per char (total): " << (static_cast<double>(total_av_score.first) / static_cast<double>(total_av_score.second)) << std::endl;
        small_score_t total_score_per_char = static_cast<small_score_t>(-total_av_score.first / static_cast<score_t>(total_av_score.second));
        std::cout << total_score_per_char << std::endl;

        total_score_per_char = 0;

        _word_ngram_tree.adjust_scores(0, total_score_per_char, 1, 1);
        _proper_tree.adjust_scores(0, total_score_per_char, 1, 1);
        _numeric_tree.adjust_scores(0, total_score_per_char, 1, 1);*/

        /*std::cout << std::endl;
        test_next_word(word_ngram_tree(), "further", 0, COMMA, "wait", "for");*/
    }

    const Word_Ngram_Tree &word_ngram_tree() const {
        return _word_ngram_tree;
    }
    const Word_Ngram_Tree &proper_tree() const {
        return _proper_tree;
    }
    const Word_Ngram_Tree &numeric_tree() const {
        return _numeric_tree;
    }
    const Word_Ngram_Tree &prefix_tree_first() const {
        return *(word_ngram_tree().find(COMMA));
    }
    const Word_Id_Map &word_id_map() const {
        return _word_id_map;
    }
private:
    class Stat_File {
    public:
        Stat_File(const std::string &filename): file(filename) {
        }
        template <class _C, class _F>
        void read(const _C &conv, const _F &proc) {
            std::string s;
            std::vector<std::pair<std::string, word_id>> words;
            while (getline(file, s)) {
                char ch = s.front();
                if (ch == '-') {
                    words.pop_back();
                }
                else {
                    size_t k = s.find(' ');
                    words.emplace_back(conv(s.substr(1, k - 1)));
                    hits_t cnt = static_cast<hits_t>(std::stoi(s.substr(k + 1)));
                    proc(words, cnt);
                    if (ch == '=') {
                        words.pop_back();
                    }
                }
            }
            assert(words.empty());
        }
    protected:
        std::ifstream file;
    };
    template <class _Conv>
    void load_stats(const std::string &filename, const _Conv &conv) {
        Stat_File f(filename);
        f.read([&](const std::string &s) -> std::pair<std::string, word_id> {
            std::string w = conv(to_lower(s));
            word_id id = _word_id_map.add(w);
            return {std::move(w), id};
        },
        [&](const auto &words, hits_t cnt) {
            _word_ngram_tree.add(_word_id_map, words, cnt, false);
            if ((words.size() <= 2) && (_word_id_map.category(words.back().second) == PROPER)) {
                _proper_tree.add(_word_id_map, words, cnt, true);
            }
            if ((words.size() <= 2) && (_word_id_map.category(words.back().second) == NUMERIC) && (words.back().second != NUMERIC)) {
                _numeric_tree.add(_word_id_map, words, cnt, true);
            }
        });
    }
    template <class _Conv>
    std::pair<std::set<std::string>, std::set<std::string>> load_stats_words(const std::vector<std::string> &stat_files, size_t limit, const _Conv &conv, const std::set<std::string> &nproper_protected, const std::set<std::string> &numeric) {
        constexpr word_id ARTICLE = 1;
        std::map<std::string, hits_t> nproper, proper;

        for(const auto &fn: stat_files) {
            std::cout << "Loading vocabulary from stat file " << fn << "...";

            Stat_File f(fn);
            f.read([&](const std::string &s) -> std::pair<std::string, word_id> {
                std::string w = conv(s);
                std::string lw = to_lower(w);
                word_id t = ((lw == "the") || (lw == "a") || (lw == "an")) ? ARTICLE : NONE;
                return {std::move(w), t};
            },
            [&](const auto &words, hits_t cnt) {
                if ((words.size() == 0) || (words.size() > 2)) {
                    return;
                }
                std::string s = words.back().first;
                std::string ls = to_lower(s);
                if (!test_word(ls)) {
                    return;
                }
                if (numeric.find(ls) != numeric.end()) {
                    return;
                }
                //1-letter words except a and i are proper names
                if ((ls.size() == 1) && (ls != "a") && (ls != "i")) {
                    proper[ls] += cnt;
                    return;
                }
                if ((s == ls) || ((words[0].second == ARTICLE) && (words.size() == 2))) {
                    nproper[ls] += cnt;
                }
                else {
                    proper[ls] += cnt;
                }
            });

            std::cout << " Done" << std::endl;
        }

        auto it_p = proper.begin();
        while (it_p != proper.end()) {
            auto it_n = nproper.find(it_p->first);
            auto p = nproper_protected.find(it_p->first);
            if (p != nproper_protected.end()) {
                nproper[it_p->first] += it_p->second;
                it_p = proper.erase(it_p);
            }
            else if (it_n != nproper.end()) {
                // more than 1/5 in lowercase => non-proper
                if (it_n->second * 4 > it_p->second) {
                    it_n->second += it_p->second;
                    it_p = proper.erase(it_p);
                }
                else {
                    it_p->second += it_n->second;
                    nproper.erase(it_n);
                }
            }
            else {
                ++it_p;
            }
        }

        /*for (const auto v: proper_list) {
            proper.erase(v);
        }*/

        auto to_set = [limit](const auto &m) {
            std::set<std::string> result;
            auto list = sort_freq<std::string>(m);
            size_t i = 0;
            for (const auto &w: list) {
                if (++i > limit) {
                    break;
                }
                result.insert(w.first);
            }
            return result;
        };


        /*auto print = [limit](const auto &m, const auto &fn) {
            std::ofstream file(fn);
            size_t i = 0;
            auto list = sort_freq<std::string>(m);
            for (const auto &w: list) {
                if (++i > limit) {
                    break;
                }
                file << w.first << " " << w.second << std::endl;
            }
        };
        print(nproper, "../xnprop.txt");
        print(proper, "../xprop.txt");*/

        return {to_set(nproper), to_set(proper)};
    }

    Word_Ngram_Tree     _proper_tree;
    Word_Ngram_Tree     _numeric_tree;
    Word_Ngram_Tree     _word_ngram_tree;
    Word_Id_Map         _word_id_map;
};
