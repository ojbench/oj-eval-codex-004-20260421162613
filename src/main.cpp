// Minimal Bookstore Management System implementation
#include <bits/stdc++.h>
using namespace std;

struct Account {
    string user;
    string pass;
    int priv = 1;
    string name;
};

struct Book {
    string isbn;
    string name;
    string author;
    string keyword;
    double price = 0.0;
    long long stock = 0;
};

static const char *ACC_FILE = "accounts.db";
static const char *BOOK_FILE = "books.db";

static bool load_accounts(unordered_map<string, Account>& accs) {
    accs.clear();
    ifstream fin(ACC_FILE);
    if (!fin.good()) return false;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        vector<string> parts; string cur;
        for (char c : line) {
            if (c == '\t') { parts.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        parts.push_back(cur);
        if (parts.size() < 4) continue;
        Account a; a.user=parts[0]; a.pass=parts[1]; a.priv=stoi(parts[2]); a.name=parts[3];
        accs[a.user]=a;
    }
    return true;
}

static void save_accounts(const unordered_map<string, Account>& accs) {
    vector<string> keys; keys.reserve(accs.size());
    for (auto &kv: accs) keys.push_back(kv.first);
    sort(keys.begin(), keys.end());
    ofstream fout(ACC_FILE, ios::trunc);
    for (auto &k: keys) {
        const auto &a = accs.at(k);
        fout << a.user << '\t' << a.pass << '\t' << a.priv << '\t' << a.name << '\n';
    }
}

static bool load_books(unordered_map<string, Book>& books) {
    books.clear();
    ifstream fin(BOOK_FILE);
    if (!fin.good()) return false;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        vector<string> parts; string cur;
        for (char c : line) {
            if (c=='\t') { parts.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        parts.push_back(cur);
        if (parts.size() < 6) continue;
        Book b; b.isbn=parts[0]; b.name=parts[1]; b.author=parts[2]; b.keyword=parts[3]; b.price=stod(parts[4]); b.stock=stoll(parts[5]);
        books[b.isbn]=b;
    }
    return true;
}

static void save_books(const unordered_map<string, Book>& books) {
    vector<string> keys; keys.reserve(books.size());
    for (auto &kv: books) keys.push_back(kv.first);
    sort(keys.begin(), keys.end());
    ofstream fout(BOOK_FILE, ios::trunc);
    fout.setf(std::ios::fixed); fout<<setprecision(2);
    for (auto &k: keys) {
        const auto &b = books.at(k);
        fout << b.isbn << '\t' << b.name << '\t' << b.author << '\t' << b.keyword << '\t'
             << b.price << '\t' << b.stock << '\n';
    }
}

static string trim(const string& s){
    size_t i=0,j=s.size();
    while (i<j && isspace((unsigned char)s[i])) ++i;
    while (j>i && isspace((unsigned char)s[j-1])) --j;
    return s.substr(i,j-i);
}

static vector<string> split_spaces(const string& s) {
    vector<string> out; string cur;
    for (size_t i=0;i<s.size();++i) {
        char c=s[i];
        if (isspace((unsigned char)c)) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

static bool parse_option(const string& s, size_t& pos, string& key, string& val) {
    // expects -key=value, where value may be "..." or bare token
    size_t n=s.size();
    while (pos<n && isspace((unsigned char)s[pos])) ++pos;
    if (pos>=n || s[pos] != '-') return false;
    ++pos;
    size_t kstart=pos;
    while (pos<n && s[pos] != '=' && !isspace((unsigned char)s[pos])) ++pos;
    if (pos>=n || s[pos] != '=') return false;
    key = s.substr(kstart, pos-kstart);
    ++pos; // skip '='
    while (pos<n && isspace((unsigned char)s[pos])) ++pos;
    if (pos>=n) { val.clear(); return true; }
    if (s[pos]=='"') {
        ++pos; size_t vstart=pos;
        while (pos<n && s[pos] != '"') ++pos;
        if (pos>=n) return false; // unmatched quote
        val = s.substr(vstart, pos-vstart);
        ++pos; // skip closing quote
    } else {
        size_t vstart=pos;
        while (pos<n && !isspace((unsigned char)s[pos])) ++pos;
        val = s.substr(vstart, pos-vstart);
    }
    return true;
}

struct LoginFrame { Account acc; string selected_isbn; bool has_selected=false; };

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    unordered_map<string, Account> accs;
    if (!load_accounts(accs)) {
        Account root{"root","sjtu",7,"root"};
        accs[root.user]=root; save_accounts(accs);
    }
    unordered_map<string, Book> books; load_books(books);

    vector<LoginFrame> stack;

    string line;
    cout.setf(std::ios::fixed); cout<<setprecision(2);
    while (true) {
        if (!getline(cin, line)) break;
        string t = trim(line);
        if (t.empty()) continue;
        vector<string> tok = split_spaces(t);
        if (tok.empty()) continue;
        string cmd = tok[0];
        auto current_priv = [&](){ return stack.empty()?0:stack.back().acc.priv; };
        auto print_invalid = [&](){ cout << "Invalid\n"; };
        auto find_acc = [&](const string& uid)->Account*{
            auto it = accs.find(uid);
            if (it==accs.end()) return nullptr; return const_cast<Account*>(&it->second);
        };

        if (cmd=="exit" || cmd=="quit") {
            break;
        } else if (cmd=="su") {
            if (tok.size()!=2 && tok.size()!=3) { print_invalid(); continue; }
            string uid=tok[1]; Account* a=find_acc(uid);
            if (!a) { print_invalid(); continue; }
            bool need_pwd=true;
            if (!stack.empty() && stack.back().acc.priv > a->priv) need_pwd=false;
            if (tok.size()==3) {
                if (a->pass != tok[2]) { print_invalid(); continue; }
                stack.push_back({*a, "", false});
            } else {
                if (need_pwd) { print_invalid(); continue; }
                stack.push_back({*a, "", false});
            }
        } else if (cmd=="logout") {
            if (tok.size()!=1) { print_invalid(); continue; }
            if (stack.empty()) { print_invalid(); continue; }
            stack.pop_back();
        } else if (cmd=="register") {
            if (tok.size()!=4) { print_invalid(); continue; }
            string uid=tok[1], pw=tok[2], uname=tok[3];
            if (accs.count(uid)) { print_invalid(); continue; }
            Account a{uid,pw,1,uname}; accs[uid]=a; save_accounts(accs);
        } else if (cmd=="passwd") {
            if (tok.size()!=3 && tok.size()!=4) { print_invalid(); continue; }
            if (stack.empty()) { print_invalid(); continue; }
            string uid=tok[1]; Account* a=find_acc(uid); if (!a) { print_invalid(); continue; }
            if (tok.size()==4) {
                if (a->pass != tok[2]) { print_invalid(); continue; }
                a->pass = tok[3]; save_accounts(accs);
            } else {
                if (current_priv()!=7) { print_invalid(); continue; }
                a->pass = tok[2]; save_accounts(accs);
            }
        } else if (cmd=="useradd") {
            if (tok.size()!=5) { print_invalid(); continue; }
            if (stack.empty() || current_priv()<3) { print_invalid(); continue; }
            string uid=tok[1], pw=tok[2], pr=tok[3], uname=tok[4];
            if (uid.empty()||pw.empty()||pr.empty()||uname.empty()) { print_invalid(); continue; }
            if (!all_of(pr.begin(), pr.end(), ::isdigit)) { print_invalid(); continue; }
            long long p=stoll(pr); if (p<0||p>7) { print_invalid(); continue; }
            if ((int)p >= current_priv()) { print_invalid(); continue; }
            if (accs.count(uid)) { print_invalid(); continue; }
            Account a{uid,pw,(int)p,uname}; accs[uid]=a; save_accounts(accs);
        } else if (cmd=="delete") {
            if (tok.size()!=2) { print_invalid(); continue; }
            if (stack.empty() || current_priv()!=7) { print_invalid(); continue; }
            string uid=tok[1]; auto it=accs.find(uid); if (it==accs.end()) { print_invalid(); continue; }
            bool is_logged=false; for (auto &fr: stack) if (fr.acc.user==uid) { is_logged=true; break; }
            if (is_logged) { print_invalid(); continue; }
            accs.erase(it); save_accounts(accs);
        } else if (cmd=="select") {
            if (tok.size()!=2) { print_invalid(); continue; }
            if (stack.empty() || current_priv()<3) { print_invalid(); continue; }
            string isbn=tok[1];
            if (!books.count(isbn)) { Book b; b.isbn=isbn; books[isbn]=b; save_books(books); }
            stack.back().selected_isbn = isbn; stack.back().has_selected = true;
        } else if (cmd=="modify") {
            if (stack.empty() || current_priv()<3) { print_invalid(); continue; }
            if (!stack.back().has_selected) { print_invalid(); continue; }
            string options = t.substr(t.find("modify")+6);
            map<string,int> seen; string key,val;
            string sel_isbn = stack.back().selected_isbn;
            Book b = books[sel_isbn];
            size_t pos=0; bool ok=true; vector<pair<string,string>> changes;
            while (true) {
                while (pos<options.size() && isspace((unsigned char)options[pos])) ++pos;
                if (pos>=options.size()) break;
                key.clear(); val.clear();
                if (!parse_option(options, pos, key, val)) { ok=false; break; }
                if (key!="ISBN" && key!="name" && key!="author" && key!="keyword" && key!="price") { ok=false; break; }
                if (seen[key]++) { ok=false; break; }
                if (val.empty()) { ok=false; break; }
                changes.push_back({key,val});
            }
            if (!ok || changes.empty()) { print_invalid(); continue; }
            Book nb = b;
            for (auto &kv: changes) {
                const string &k=kv.first; const string &v=kv.second;
                if (k=="ISBN") {
                    if (v==sel_isbn) { ok=false; break; }
                    if (books.count(v)) { ok=false; break; }
                    nb.isbn = v;
                } else if (k=="name") {
                    nb.name = v;
                } else if (k=="author") {
                    nb.author = v;
                } else if (k=="keyword") {
                    vector<string> segs; string cur; set<string> st;
                    for (size_t i=0;i<=v.size();++i) {
                        if (i==v.size() || v[i]=='|') {
                            if (cur.empty()) { ok=false; break; }
                            if (!st.insert(cur).second) { ok=false; break; }
                            segs.push_back(cur); cur.clear();
                        } else cur.push_back(v[i]);
                    }
                    if (!ok) break;
                    nb.keyword = v;
                } else if (k=="price") {
                    try { nb.price = stod(v); if (nb.price < 0) { ok=false; break; } } catch(...) { ok=false; break; }
                }
            }
            if (!ok) { print_invalid(); continue; }
            if (nb.isbn != b.isbn) {
                books.erase(b.isbn);
            }
            books[nb.isbn]=nb; save_books(books);
            stack.back().selected_isbn = nb.isbn; stack.back().has_selected=true;
        } else if (cmd=="import") {
            if (tok.size()!=3) { print_invalid(); continue; }
            if (stack.empty() || current_priv()<3) { print_invalid(); continue; }
            if (!stack.back().has_selected) { print_invalid(); continue; }
            string qs=tok[1], cs=tok[2];
            if (!all_of(qs.begin(), qs.end(), ::isdigit)) { print_invalid(); continue; }
            long long q = stoll(qs); if (q<=0) { print_invalid(); continue; }
            double total; try { total = stod(cs); if (!(total>0)) { print_invalid(); continue; } } catch (...) { print_invalid(); continue; }
            Book &b = books[stack.back().selected_isbn];
            b.stock += q; save_books(books);
        } else if (cmd=="buy") {
            if (tok.size()!=3) { print_invalid(); continue; }
            if (stack.empty() || current_priv()<1) { print_invalid(); continue; }
            string isbn=tok[1], qs=tok[2];
            if (!all_of(qs.begin(), qs.end(), ::isdigit)) { print_invalid(); continue; }
            long long q=stoll(qs); if (q<=0) { print_invalid(); continue; }
            auto it=books.find(isbn); if (it==books.end()) { print_invalid(); continue; }
            Book &b = it->second; if (b.stock < q) { print_invalid(); continue; }
            b.stock -= q; save_books(books);
            double total = b.price * (double)q;
            cout<< fixed << setprecision(2) << total << "\n";
        } else if (cmd=="show") {
            if (stack.empty() || current_priv()<1) { print_invalid(); continue; }
            if (tok.size()==1) {
                vector<string> keys; keys.reserve(books.size());
                for (auto &kv: books) keys.push_back(kv.first);
                sort(keys.begin(), keys.end());
                for (auto &k: keys) {
                    const Book &b = books[k];
                    cout<<b.isbn<<"\t"<<b.name<<"\t"<<b.author<<"\t"<<b.keyword<<"\t"<<fixed<<setprecision(2)<<b.price<<"\t"<<b.stock<<"\n";
                }
                if (books.empty()) cout<<"\n";
            } else {
                string options = t.substr(t.find("show")+4);
                string key,val; size_t pos=0; bool ok=parse_option(options,pos,key,val);
                if (!ok) { print_invalid(); continue; }
                while (pos<options.size() && isspace((unsigned char)options[pos])) ++pos;
                if (pos<options.size()) { print_invalid(); continue; }
                if (val.empty()) { print_invalid(); continue; }
                vector<const Book*> result;
                if (key=="ISBN") {
                    auto it=books.find(val); if (it!=books.end()) result.push_back(&it->second);
                } else if (key=="name") {
                    for (auto &kv: books) if (kv.second.name==val) result.push_back(&kv.second);
                } else if (key=="author") {
                    for (auto &kv: books) if (kv.second.author==val) result.push_back(&kv.second);
                } else if (key=="keyword") {
                    if (val.find('|')!=string::npos) { print_invalid(); continue; }
                    for (auto &kv: books) {
                        const string &kw = kv.second.keyword;
                        string cur; bool hit=false;
                        for (size_t i=0;i<=kw.size();++i) {
                            if (i==kw.size() || kw[i]=='|') {
                                if (cur==val) { hit=true; break; }
                                cur.clear();
                            } else cur.push_back(kw[i]);
                        }
                        if (hit) result.push_back(&kv.second);
                    }
                } else { print_invalid(); continue; }
                sort(result.begin(), result.end(), [](const Book* a, const Book* b){ return a->isbn < b->isbn; });
                for (auto *bp: result) {
                    const Book &b = *bp;
                    cout<<b.isbn<<"\t"<<b.name<<"\t"<<b.author<<"\t"<<b.keyword<<"\t"<<fixed<<setprecision(2)<<b.price<<"\t"<<b.stock<<"\n";
                }
                if (result.empty()) cout<<"\n";
            }
        } else {
            print_invalid();
        }
    }
    return 0;
}
