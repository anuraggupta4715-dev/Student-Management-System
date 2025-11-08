#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <iomanip>
#include <cctype>
using namespace std;
inline void clearCinLine() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
inline string readLine(const string& prompt) {
    cout << prompt;
    string s;
    getline(cin >> ws, s);
    return s;
}
inline int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        int x;
        if (cin >> x) { clearCinLine(); return x; }
        cout << "Invalid number.\n";
        clearCinLine();
    }
}
inline double readDouble(const string& prompt) {
    while (true) {
        cout << prompt;
        double x;
        if (cin >> x) { clearCinLine(); return x; }
        cout << "Invalid number.\n";
        clearCinLine();
    }
}
inline bool readYesNo(const string& prompt) {
    while (true) {
        cout << prompt << " (y/n): ";
        char c;
        if (cin >> c) {
            clearCinLine();
            c = static_cast<char>(tolower(c));
            if (c=='y') return true;
            if (c=='n') return false;
        } else clearCinLine();
        cout << "Only y or n allowed.\n";
    }
}
struct ISerializable {
    virtual ~ISerializable() = default;
    virtual string serialize() const = 0;
};
class Person : public ISerializable {
protected:
    string name_;
    int age_{};
    static void ensureValidAge(int a) { if (a < 0 || a > 130) throw invalid_argument("Invalid age."); }
public:
    Person(string n, int a) : name_(move(n)), age_(a) {
        if (name_.empty()) throw invalid_argument("Name empty");
        ensureValidAge(a);
    }
    virtual ~Person() = default;
    const string& name() const { return name_; }
    int age() const { return age_; }
    void setName(const string& n) { if (n.empty()) throw invalid_argument("Empty name"); name_ = n; }
    void setAge(int a) { ensureValidAge(a); age_ = a; }
    virtual string role() const = 0;
    virtual void print(ostream& os) const { os << "Name: " << name_ << "\nAge: " << age_ << "\n"; }
};
struct Address {
    string line1, city, state, zip;
    string toString() const {
        string s = line1;
        if (!city.empty()) s += ", " + city;
        if (!state.empty()) s += ", " + state;
        if (!zip.empty()) s += " (" + zip + ")";
        return s;
    }
};
struct Course {
    string code;
    string title;
};
class Student : public Person {
protected:
    int roll_;
    Address address_;
    vector<Course> courses_;
    static int& rollSeed() { static int seed = 1000; return seed; }
public:
    Student(string n, int a, int r) : Person(move(n), a), roll_(r) {
        if (r <= 0) throw invalid_argument("Invalid roll");
    }
    Student(string n, int a) : Person(move(n), a), roll_(++rollSeed()) {}
    int roll() const { return roll_; }
    void setRoll(int r) { if (r <= 0) throw invalid_argument("Invalid roll"); roll_ = r; }
    void setAddress(Address a) { address_ = move(a); }
    const Address& address() const { return address_; }
    void addCourse(const Course& c) {
        auto it = find_if(courses_.begin(), courses_.end(), [&](const Course& x){ return x.code == c.code; });
        if (it == courses_.end()) courses_.push_back(c);
    }
    bool removeCourseByCode(const string& code) {
        auto it = remove_if(courses_.begin(), courses_.end(), [&](const Course& c){ return c.code == code; });
        if (it != courses_.end()) { courses_.erase(it, courses_.end()); return true; }
        return false;
    }
    string role() const override { return "Student"; }
    void print(ostream& os) const override {
        os << "[ " << role() << " ]\n";
        Person::print(os);
        os << "Roll No: " << roll_ << "\n";
        if (!address_.toString().empty()) os << "Address: " << address_.toString() << "\n";
        if (!courses_.empty()) {
            os << "Courses: ";
            for (size_t i = 0; i < courses_.size(); i++) {
                os << courses_[i].code;
                if (i + 1 < courses_.size()) os << ", ";
            }
            os << "\n";
        }
    }
    string serialize() const override {
        return "Student|" + to_string(roll_) + "|" + name_ + "|" + to_string(age_);
    }
};
class HonorsStudent final : public Student {
    double scholarship_;
public:
    HonorsStudent(string n, int a, int r, double s) : Student(move(n), a, r), scholarship_(s) {
        if (s < 0) throw invalid_argument("Invalid scholarship");
    }
    string role() const override { return "Honors Student"; }
    void print(ostream& os) const override {
        Student::print(os);
        os << "Scholarship: " << fixed << setprecision(2) << scholarship_ << "\n";
    }
    string serialize() const override { return Student::serialize() + "|SCH:" + to_string(scholarship_); }
};
inline ostream& operator<<(ostream& os, const Person& p) { p.print(os); return os; }

class StudentRepository {
    vector<unique_ptr<Student>> data_;
public:
    void add(unique_ptr<Student> s) {
        if (findByRoll(s->roll())) throw runtime_error("Duplicate roll");
        data_.push_back(move(s));
    }
    Student* findByRoll(int r) {
        for (auto& p : data_) if (p->roll() == r) return p.get();
        return nullptr;
    }
    vector<Student*> findByName(const string& n) {
        vector<Student*> res;
        for (auto& p : data_) if (p->name() == n) res.push_back(p.get());
        return res;
    }
    bool removeByRoll(int r) {
        auto it = remove_if(data_.begin(), data_.end(), [&](auto& p){ return p->roll() == r; });
        if (it != data_.end()) { data_.erase(it, data_.end()); return true; }
        return false;
    }
    void sortByRoll() {
        sort(data_.begin(), data_.end(), [](auto& a, auto& b){ return a->roll() < b->roll(); });
    }
    const auto& all() const { return data_; }
};
class App {
    StudentRepository repo_;
    static Address enterAddress() {
        Address a;
        a.line1 = readLine("Address line: ");
        a.city  = readLine("City: ");
        a.state = readLine("State: ");
        a.zip   = readLine("ZIP: ");
        return a;
    }
public:
    void addStudent(bool honors) {
        try {
            string name = readLine("Enter name: ");
            int age = readInt("Enter age: ");
            int roll = readYesNo("Manual roll?") ? readInt("Roll: ") : 0;
            unique_ptr<Student> s;
            if (honors) {
                if (roll <= 0) roll = readInt("Enter roll: ");
                double sch = readDouble("Scholarship: ");
                s = make_unique<HonorsStudent>(name, age, roll, sch);
            } else {
                s = (roll > 0) ? make_unique<Student>(name, age, roll) : make_unique<Student>(name, age);
            }
            if (readYesNo("Add address?")) s->setAddress(enterAddress());
            repo_.add(move(s));
        } catch (const exception& e) { cout << "Error: " << e.what() << "\n"; }
    }
    void displayAll() const {
        if (repo_.all().empty()) { cout << "No students.\n"; return; }
        for (auto& p : repo_.all()) cout << *p << "\n";
    }
    void search() {
        int opt = readInt("1) Roll 2) Name: ");
        if (opt == 1) {
            int r = readInt("Roll: ");
            if (auto s = repo_.findByRoll(r)) cout << *s; else cout << "Not found.\n";
        } else {
            string n = readLine("Name: ");
            auto list = repo_.findByName(n);
            if (list.empty()) cout << "Not found.\n";
            else for (auto s : list) cout << *s << "\n";
        }
    }
    void update() {
        int r = readInt("Enter roll: ");
        Student* s = repo_.findByRoll(r);
        if (!s) { cout << "Not found.\n"; return; }
        int op = readInt("1) Name 2) Age 3) Roll 4) Address: ");
        try {
            if (op == 1) s->setName(readLine("New name: "));
            else if (op == 2) s->setAge(readInt("New age: "));
            else if (op == 3) s->setRoll(readInt("New roll: "));
            else if (op == 4) s->setAddress(enterAddress());
            else cout << "Invalid.\n";
            cout << *s;
        } catch (const exception& e) { cout << "Error: " << e.what(); }
    }
    void remove() {
        int r = readInt("Roll to delete: ");
        if (repo_.removeByRoll(r)) cout << "Deleted.\n"; else cout << "Not found.\n";
    }
    void run() {
        bool run = true;
        while (run) {
            cout << "\n1) Add\n2) Add Honors\n3) Show\n4) Search\n5) Update\n6) Delete\n7) Sort\n8) Exit\n";
            int op = readInt("Choice: ");
            if (op == 1) addStudent(false);
            else if (op == 2) addStudent(true);
            else if (op == 3) displayAll();
            else if (op == 4) search();
            else if (op == 5) update();
            else if (op == 6) remove();
            else if (op == 7) { repo_.sortByRoll(); cout << "Sorted.\n"; }
            else if (op == 8) run = false;
            else cout << "Invalid.\n";
        }
    }
};
int main() {
    App app;
    app.run();
    return 0;
}
