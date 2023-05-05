#pragma once
#include <iostream>
#include <vector>

inline void print() {
    std::cout << std::endl;
}

template<typename T>
inline std::ostream& operator<<(std::ostream& stream, std::vector<T> vector) { 
    stream << "Vector{";
    
    for (int i = 0; i < (signed) vector.size(); i++) {
        stream << vector[i];
        stream << (i == (signed) vector.size() - 1 ? "}" : ", ");
    }
    
    if (!vector.size()) 
        stream << "}";

    return stream;
}

template<typename First, typename ... Values>
inline void print(First arg, const Values&... rest) {
    std::cout << arg << " ";
    print(rest...);
}
