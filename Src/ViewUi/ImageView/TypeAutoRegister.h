#ifndef TYPEAUTOREGISTER_H
#define TYPEAUTOREGISTER_H

#include <map>
#include <vector>
#include "Global.h"

//先定义公场类，用于生成不同的对象
class TypeAutoRegister
{
public:
    using Parser = std::function<void(cv::Rect&, std::vector<cv::Point>&)>;

    static TypeAutoRegister& Instance()
    {
        static TypeAutoRegister instance;
        return instance;
    }

    template<typename T>
    void Register(DefectType type)
    {
        m_parsers[type] = [this](cv::Rect& rect, std::vector<cv::Point>& vertexs)
        {
            static T parser;
            parser.Parse(m_cols, m_rows, rect, vertexs);
        };
    }

    void Parse(DefectType type, cv::Rect& rect, std::vector<cv::Point>& vertexs)
    {
        auto it = m_parsers.find(type);
        if (it != m_parsers.end())
        {
            it->second(rect, vertexs);
        }
    }

    void SetWH(int cols, int rows)
    {
        m_cols = cols;
        m_rows = rows;
    }

private:
    TypeAutoRegister() = default;

private:
    std::map<DefectType, Parser> m_parsers;
    int                          m_cols;
    int                          m_rows;
};

template <typename T>
class TypeAutoRegisterHelper
{
public:
    TypeAutoRegisterHelper(DefectType type)
    {
        TypeAutoRegister::Instance().Register<T>(type);
    }
};

#define REGISTER_TYPE(parse_class, type) \
    static TypeAutoRegisterHelper<parse_class> g_##parse_class##_register(type);

template <typename Derived>
class TypeBase
{
public:
    void Parse(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs)
    {
        static_cast<Derived*>(this)->ParseImpl(cols, rows, rect, vertexs);
    }

protected:
    void ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs) = delete;
};

class Type0: public TypeBase<Type0>
{
public:
    void ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs);
};
REGISTER_TYPE(Type0, DefectType::TYPE_POORCOATING);

class Type1 : public TypeBase<Type1>
{
public:
    void ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs);
};
REGISTER_TYPE(Type1, DefectType::TYPE_SCRATCH);

class Type2 : public TypeBase<Type2>
{
public:
    void ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs);
};
REGISTER_TYPE(Type2, DefectType::TYPE_CALCULUS);

class Type3 : public TypeBase<Type3>
{
public:
    void ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs);
};
REGISTER_TYPE(Type3, DefectType::TYPE_BUBBLE);

class Type4 : public TypeBase<Type4>
{
public:
    void ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs);
};
REGISTER_TYPE(Type4, DefectType::TYPE_TRADEMARK);

class Type5 : public TypeBase<Type5>
{
public:
    void ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs);
};
REGISTER_TYPE(Type5, DefectType::TYPE_WATERSTAIN);

class Type6 : public TypeBase<Type6>
{
public:
    void ParseImpl(int cols, int rows, cv::Rect& rect, std::vector<cv::Point>& vertexs);
};
REGISTER_TYPE(Type6, DefectType::TYPE_SMUDGE);

#endif
