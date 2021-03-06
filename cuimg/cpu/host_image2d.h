#ifndef CUIMG_HOST_IMAGE2D_H_
# define CUIMG_HOST_IMAGE2D_H_

# include <boost/shared_ptr.hpp>
#ifndef NO_OPENMP
# include <omp.h>
#endif

# include <cuimg/target.h>
# include <cuimg/concepts.h>
# include <cuimg/dsl/expr.h>
# include <cuimg/point2d.h>
# include <cuimg/obox2d.h>
# include <cuimg/gpu/kernel_image2d.h>

#ifdef WITH_OPENCV
# include <opencv2/core/core.hpp>
#endif


namespace cuimg
{
  struct cpu;
  template <typename V>
  class kernel_image2d;

  template <typename V>
  class host_image2d : public Image2d<host_image2d<V> >
  {
  public:
    static const cuimg::target target = CPU;
    typedef cpu architecture;

    typedef int is_expr;
    typedef V value_type;
    typedef point2d<int> point;
    typedef obox2d domain_type;
    typedef boost::shared_ptr<V> PT;
    typedef kernel_image2d<V> kernel_type;

    host_image2d();
    ~host_image2d();
    host_image2d(unsigned nrows, unsigned ncols, unsigned border = 0, bool pinned = 0);
    host_image2d(V* data, unsigned nrows, unsigned ncols, unsigned pitch);
    host_image2d(const domain_type& d, unsigned border = 0, bool pinned = 0);

    host_image2d(const host_image2d<V>& d);
    void swap(host_image2d<V>& o);

#ifdef WITH_OPENCV
    host_image2d(IplImage * imgIpl);
    host_image2d(cv::Mat m);
#endif
    host_image2d<V>& operator=(const host_image2d<V>& d);

    const domain_type& domain() const;
    int nrows() const;
    int ncols() const;
    bool has(const point& p) const;

    size_t pitch() const;
    int border() const;

    V& operator[](int i);
    const V& operator[](int i) const;

    V& operator()(const point& p);
    const V& operator()(const point& p) const;

    V& operator()(int r, int c);
    const V& operator()(int r, int c) const;

    inline V& eval(const point& p) { return (*this)(p); };
    inline const V& eval(const point& p) const { return (*this)(p); };

    operator bool() const { return data_.get(); }

#ifdef WITH_OPENCV
    operator cv::Mat() const;
    IplImage* getIplImage() const;
    host_image2d<V>& operator=(IplImage * imgIpl);
    host_image2d<V>& operator=(cv::Mat imgIpl);
#endif

    V* row(int i);
    const V* row(int i) const;

    V* data();
    const V* data() const;
    inline V* begin() const;
    inline V* end() const;

    size_t buffer_size() const;

    inline i_int2 index_to_point(int idx) const;
    inline int point_to_index(const point& p) const;

    template <typename E>
    host_image2d<V>& operator=(const expr<E>& e)
    {
      const E& x(*(E*)&e);
      //#pragma omp parallel for schedule(static, 2)
      for (unsigned r = 0; r < nrows(); r++)
        for (unsigned c = 0; c < ncols(); c++)
          (*this)(r, c) = x.eval(point2d<int>(r, c));
      return *this;
    }

  private:
    void allocate(const domain_type& d, unsigned border, bool pinned);

    domain_type domain_;
    int pitch_;
    int border_;
    boost::shared_ptr<V> data_;
    V* begin_;
  };

  template <typename T>
  host_image2d<T> clone(const host_image2d<T>& i)
  {
    host_image2d<T> n(i.domain());
    copy(i, n);
    return n;
  }

  template <typename T>
  struct return_type;

  template <typename V>
  struct return_type<host_image2d<V> >
  {
    typedef typename host_image2d<V>::value_type ret;
  };

}
# include <cuimg/cpu/host_image2d.hpp>

#endif
