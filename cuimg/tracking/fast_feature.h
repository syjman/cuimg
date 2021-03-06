#ifndef CUIMG_FAST_FEATURE_H_
# define CUIMG_FAST_FEATURE_H_

# include <cuimg/gpu/device_image2d.h>
# include <cuimg/gpu/kernel_image2d.h>
# include <cuimg/point2d.h>
# include <cuimg/obox2d.h>
# include <cuimg/improved_builtin.h>

namespace cuimg
{

  struct __align__(8) dfast
  {
    float max_diff;
    float intensity;
    char orientation;
    char sign;
  };

  __host__ __device__ inline
  dfast operator+(const dfast& a, const dfast& b);
  __host__ __device__ inline
  dfast operator-(const dfast& a, const dfast& b);

  template <typename S>
  __host__ __device__ inline
  dfast operator/(const dfast& a, const S& s);

  template <typename S>
  __host__ __device__ inline
  dfast operator*(const dfast& a, const S& s);

  class kernel_fast_feature;

  class fast_feature
  {
  public:
    typedef dfast feature_t;
    typedef obox2d domain_t;

    typedef kernel_fast_feature kernel_type;

    inline fast_feature(const domain_t& d);

    inline void update(const image2d_f4& in);

    inline const domain_t& domain() const;

    inline image2d_D& previous_frame();
    inline image2d_D& current_frame();
    inline image2d_f1& pertinence();

    const image2d_f4& feature_color() const;


  private:
    inline void swap_buffers();


    image2d_f1 gl_frame_;
    image2d_f1 blurred_;
    image2d_f1 tmp_;

    image2d_f1 pertinence_;

    image2d_D f1_;
    image2d_D f2_;

    image2d_D* f_prev_;
    image2d_D* f_;

    image2d_f4 fast_color_;

    float grad_thresh;
  };

  class kernel_fast_feature
  {
  public:
    typedef dfast feature_t;

    inline kernel_fast_feature(fast_feature& f);


    inline
    __device__ float distance(const point2d<int>& p_prev,
                              const point2d<int>& p_cur);

    inline
    float distance(const dfast& a, const dfast& b);

    __device__ inline
    kernel_image2d<dfast>& previous_frame();
    __device__ inline
    kernel_image2d<dfast>& current_frame();
    __device__ inline
    kernel_image2d<i_float1>& pertinence();

  private:
    kernel_image2d<i_float1> pertinence_;
    kernel_image2d<dfast> f_prev_;
    kernel_image2d<dfast> f_;
  };

}

# include <cuimg/tracking/fast_feature.hpp>

#endif // ! CUIMG_FAST_FEATURE_H_
