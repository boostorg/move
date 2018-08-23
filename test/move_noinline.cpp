#include <boost/move/core.hpp>
#include <boost/move/move.hpp>
#include <boost/core/lightweight_test.hpp>

class movable_v2 {
public:
    movable_v2() : value_(1) {}

private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(movable_v2)

public:
    movable_v2(BOOST_RV_REF(movable_v2) m)
    {
        value_ = m.value_;
        m.value_ = 0;
    }
    movable_v2& operator=(BOOST_RV_REF(movable_v2) m);

    bool moved() const
    { return !value_; }

private:
    int value_;
};

movable_v2& movable_v2::operator=(BOOST_RV_REF(movable_v2) m)
{
    if (this != &m) {
        value_ = m.value_;
        m.value_ = 0;
    }
    return *this;
}

namespace boost{

template<>
struct has_nothrow_move<movable_v2>
{
    static const bool value = true;
};

} // namespace boost

int main()
{
    movable_v2 m;
    movable_v2 m2;

    m2 = ::boost::move(m);
    BOOST_TEST(m.moved());
    BOOST_TEST_NOT(m2.moved());
    return 0;
}
