#ifndef CUIMG_FFAST4_FEATURE_HPP_
# define CUIMG_FFAST4_FEATURE_HPP_

# ifdef WITH_OPENCV
#  include <opencv/cv.h>
#  include <opencv/highgui.h>
# endif

# include <cuimg/gpu/cuda.h>

# include <cmath>

# include <cuimg/copy.h>
# include <cuimg/pw_call.h>
# include <cuimg/gl.h>
# include <cuimg/neighb2d_data.h>
# include <cuimg/gpu/local_jet_static.h>
# include <cuimg/dsl/binary_div.h>
# include <cuimg/dsl/binary_mul.h>
# include <cuimg/dsl/binary_add.h>
# include <cuimg/dsl/get_comp.h>
# include <cuimg/meta_gaussian/meta_gaussian_coefs_1.h>
# include <cuimg/meta_gaussian/meta_gaussian_coefs_2.h>
# include <cuimg/meta_gaussian/meta_gaussian_coefs_4.h>
# include <cuimg/meta_gaussian/meta_gaussian_coefs_3.h>
# include <cuimg/meta_gaussian/meta_gaussian_coefs_6.h>
# include <cuimg/tracking/fast_tools.h>
# include <cuimg/gpu/texture.h>

# include <cuimg/dige.h>

# ifndef NVCC
#  include <emmintrin.h>
# endif

#include <dige/widgets/image_view.h>

using dg::dl;
using namespace dg::widgets;

namespace cuimg
{

  #define s1_tex UNIT_STATIC(haxb)
  #define s2_tex UNIT_STATIC(xyhg)

#ifdef NVCC
  texture<float1, cudaTextureType2D, cudaReadModeElementType> s1_tex;
  texture<float1, cudaTextureType2D, cudaReadModeElementType> s2_tex;
#else
  int s1_tex;
  int s2_tex;
#endif

  // inline
  // __host__ __device__
  // float distance_mean(const dffast4& a, const dffast4& b)
  // {
  //   if (::abs(a.pertinence - b.pertinence) > 0.2f ||
  //       b.pertinence < 0.1f ||
  //       a.pertinence < 0.1f
  //       )
  //     return 99999.f;
  //   else
  //   {
  //     int d = 0;
  //     for (unsigned i = 0; i < 16; i++)
  //     {
  //       int tmp = int(a.distances[i]) - int(b.distances[i]);
  //       d += tmp * tmp;
  //     }

  //     return ::sqrt(float(d)) / ::sqrt(255.f * 16.f);
  //   }
  // }


#if __CUDA_ARCH__ >= 200
  inline
  __host__ __device__
  float distance_mean_linear(const dffast4& a, const dffast4& b)
  {
    int d = 0;
    for (char i = 0; i < 16; i++)
      d += ::abs(int(a[i]) - int(b[i]));

    return d / (255.f * 16.f);
  }
#else
  inline
  __host__ __device__
  float distance_mean_linear(const dffast4& a, const dffast4& b)
  {
    float d = 0;
    for (char i = 0; i < 16; i++)
      d += ::abs(float(a[i]) - float(b[i]));

    return d / (255.f * 16.f);
  }
#endif

  inline
  __host__ __device__
  float distance_mean_linear_s2(const dffast4& a, const dffast4& b)
  {
    int d = 0;
    for (unsigned i = 8; i < 16; i++)
    {
      int tmp = int(a.distances[i]) - int(b.distances[i]);
      d += tmp * tmp;
    }

    return ::sqrt(float(d)) / ::sqrt(255.f * 8.f);
  }

  inline
  __host__ __device__
  float distance_max_linear(const dffast4& a, const dffast4& b)
  {
    int d = 0;
    for (unsigned i = 0; i < 16; i++)
    {
      int tmp = ::abs(int(a.distances[i]) - int(b.distances[i]));
      if (tmp > d)
        d = tmp;
    }

    return d;
  }


  inline
  __host__ __device__
  float distance_mean_s2(const dffast4& a, const dffast4& b)
  {
    int d = 0;
    for (unsigned i = 8; i < 16; i++)
    {
      int tmp = int(a.distances[i]) - int(b.distances[i]);
      d += tmp * tmp;
    }

    return ::sqrt(float(d)) / ::sqrt(255.f * 8.f);
  }

  __host__ __device__ inline
  dffast4 operator+(const dffast4& a, const dffast4& b)
  {
    dffast4 res;
    for (unsigned i = 0; i < 16; i++)
      res.distances[i] = a.distances[i] + b.distances[i];
    //res.pertinence = a.pertinence + b.pertinence;
    return res;
  }

  __host__ __device__ inline
  dffast4 operator-(const dffast4& a, const dffast4& b)
  {
    dffast4 res;
    for (unsigned i = 0; i < 16; i++)
      res.distances[i] = a.distances[i] - b.distances[i];
    //res.pertinence = a.pertinence - b.pertinence;
    return res;
  }

  template <typename S>
  __host__ __device__ inline
  dffast4 operator/(const dffast4& a, const S& s)
  {
    dffast4 res;
    for (unsigned i = 0; i < 16; i++)
      res.distances[i] = a.distances[i] / s;
    //res.pertinence = a.pertinence / s;
    return res;
  }

  template <typename S>
  __host__ __device__ inline
  dffast4 operator*(const dffast4& a, const S& s)
  {
    dffast4 res;
    for (unsigned i = 0; i < 16; i++)
      res.distances[i] = a.distances[i] * s;
    //res.pertinence = a.pertinence * s;
    return res;
  }

  __host__ __device__ inline
  dffast4 weighted_mean(const dffast4& a, float aw, const dffast4& b, float bw)
  {
    dffast4 res;
    for (unsigned i = 0; i < 16; i++)
      res.distances[i] = (float(a.distances[i]) * aw + float(b.distances[i]) * bw) / (aw + bw);
    //res.pertinence = (a.pertinence * aw + b.pertinence * bw) / (aw + bw);
    return res;
  }

   __constant__ const int circle_r3_ffast38s1[8][2] = {
     {-3, 0}, {-2, 2},
     { 0, 3}, { 2, 2},
     { 3, 0}, { 2,-2},
     { 0,-3}, {-2,-2}
   };
   __constant__ const int circle_r3_ffast38s2[8][2] = {
     {-9, 0}, {-6, 6},
     { 0, 9}, { 6, 6},
     { 9, 0}, { 6,-6},
     { 0,-9}, {-6,-6}
   };
  /*
  __constant__ const int circle_r3_ffast4[8][2] = {
    {-1, 0}, {-1, 1},
    { 0, 1}, { 1, 1},
    { 1, 0}, { 1,-1},
    { 0,-1}, {-1,-1}
  };
  */


  template <typename V>
  __global__ void filter_pertinence(kernel_image2d<i_float1> pertinence,
                                    kernel_image2d<i_float1> out)
  {
    point2d<int> p = thread_pos2d();
    if (!pertinence.has(p))
      return;



    if (p.row() < 3 || p.row() > pertinence.domain().nrows() - 3 ||
        p.col() < 3 || p.col() > pertinence.domain().ncols() - 3)
    {
      out(p).x = 0.f;
      return;
    }

    float max = pertinence(p).x;

    //for_all_in_static_neighb2d(p, n, c25)
    for(unsigned i = 0; i < 25; i++)
    {
      // point2d<int> n(p.row() + c25[i][0],
      //                p.col() + c25[i][1]);
      float vn = pertinence(p.row() + c25[i][0],
                            p.col() + c25[i][1]);
      if (//pertinence.has(n) &&
          max < vn)
        max = vn;
      //max = ::max(max, pertinence(n).x);
   }

    if (max > 0.3f)
      out(p) = pertinence(p).x;
    else
      out(p) = 0.f;
  }


#define FFAST4_sig(T, V)                      \
  kernel_image2d<i_float4>,                     \
  kernel_image2d<V>,                            \
    kernel_image2d<V>,                          \
    kernel_image2d<i_float1>,                   \
    float,                                      \
    &FFAST4<V>

  template <typename V>
  __device__ void FFAST4(thread_info<GPU> ti,
                            kernel_image2d<i_float4> frame_color,
                            kernel_image2d<V> frame_s1,
                            kernel_image2d<V> frame_s2,
                            kernel_image2d<i_float1> pertinence,
                            float grad_thresh)
  {
    point2d<int> p = thread_pos2d(ti);
    if (!frame_s1.has(p))//; || track(p).x == 0)
      return;


    if (p.row() < 6 || p.row() >= pertinence.domain().nrows() - 6 ||
        p.col() < 6 || p.col() >= pertinence.domain().ncols() - 6)
    {
      pertinence(p).x = 0.f;
      return;
    }

    float pv;

    {
      float min_diff = 9999999.f;
      float max_single_diff = 0.f;
      pv = tex2D(flag<GPU>(), s1_tex, frame_s1, p).x;
      int sign = 0;
      for(int i = 0; i < 8; i++)
      {

        gl01f v1 = tex2D(flag<GPU>(), s1_tex, frame_s1,
			   p.col() + circle_r3[i][1],
			   p.row() + circle_r3[i][0]).x;

        gl01f v2 = tex2D(flag<GPU>(), s1_tex, frame_s1,
			   p.col() + circle_r3[(i+8)][1],
			   p.row() + circle_r3[(i+8)][0]).x;

        {
          float diff = pv -
            (v1 + v2) / 2.f;


          float adiff = fabs(diff);

          if (adiff < min_diff)
	  {
            min_diff = adiff;

	    if (min_diff < 0.01)
	    {
	      min_diff = 0;
	      break;
	    }
	  }
          if (max_single_diff < adiff) max_single_diff = adiff;
        }
      }

      pv = tex2D(flag<GPU>(), s2_tex, frame_s2, p.col()/2, p.row()/2).x;
      float min_diff_large = 9999999.f;
      float max_single_diff_large = 0.f;
      //int min_orientation_large;
      for(int i = 0; i < 8; i++)
      {

        gl01f v1 = tex2D(flag<GPU>(), s2_tex, frame_s2,
			   p.col()/2 + circle_r3[i][1],
			   p.row()/2 + circle_r3[i][0]);

        gl01f v2 = tex2D(flag<GPU>(), s2_tex, frame_s2,
			   p.col()/2 + circle_r3[(i+8)][1],
			   p.row()/2 + circle_r3[(i+8)][0]);

        {
          float diff = pv - (v1 + v2) / 2.f;
          float adiff = fabs(diff);

          if (adiff < min_diff_large)
          {
            min_diff_large = adiff;
	    if (min_diff_large < 0.01)
	    {
	      min_diff_large = 0;
	      break;
	    }
          }

          if (max_single_diff_large < adiff) max_single_diff_large = adiff;
        }

      }


      if (min_diff < min_diff_large)
      {
        min_diff = min_diff_large;
        max_single_diff = max_single_diff_large;
      }


      if (max_single_diff >= grad_thresh)
      {
        min_diff = min_diff / max_single_diff;
      }
      else
        min_diff = 0;

      pertinence(p) = min_diff;

    }

  }
  /*
  template <typename V>
  void FFAST4(thread_info<CPU> ti,
                 kernel_image2d<i_float4> frame_color,
                 kernel_image2d<V> frame_s1,
                 kernel_image2d<V> frame_s2,
                 // kernel_image2d<dffast4> out,
                 kernel_image2d<i_float1> pertinence,
                 float grad_thresh)
  {

    point2d<int> p = thread_pos2d(ti);
    if (!frame_s1.has(p))//; || track(p).x == 0)
      return;


    if (p.row() < 6 || p.row() >= pertinence.domain().nrows() - 6 ||
        p.col() < 6 || p.col() >= pertinence.domain().ncols() - 6)
    {
      pertinence(p).x = 0.f;
      return;
    }


    gl01f pv;

    {
      float min_diff = 9999999.f;
      float max_single_diff = 0.f;
      pv = V(tex2D(flag<CPU>(), s1_tex, frame_s1, p));

      gl01f ns[16];

      for(int i = 0; i < 16; i++)
	ns[i] = V(tex2D(flag<CPU>(), s1_tex, frame_s1,
			     p.col() + circle_r3_h[i][1],
			     p.row() + circle_r3_h[i][0]));

      float v1 = ns[15];
      float v2 = ns[7];

      for(int i = 0; i < 8; i++)
      {
	v1 = (2.f*v1 + ns[i]) / 3.f;
	v2 = (2.f*v2 + ns[i + 8]) / 3.f;

	// v1 = ns[i];
	// v2 = ns[i + 8];

	float diff1 = v1 - v2;
	float adiff1 = fabs(v1 - v2);

	float coef;
	// if (adiff2 > adiff1) coef = adiff2 / adiff1;
	// else
	//   coef = adiff1 / adiff2;
	// coef = 1.f / (coef);

	coef = adiff1;

	if (min_diff > coef) min_diff = coef;
	// if (adiff1 < min_diff)
	//   min_diff = adiff1;
	// if (adiff2 < min_diff)
	//   min_diff = adiff2;

	// if (min_diff < 0.001f) break;

	if (max_single_diff < adiff1) max_single_diff = adiff1;
	// if (max_single_diff < adiff2) max_single_diff = adiff2;

      }

      if (max_single_diff >= grad_thresh)
      {
        min_diff = min_diff / max_single_diff;
      }
      else
        min_diff = 0;

      pertinence(p) = min_diff *2.f;
      // pertinence(p) = p.col() / float(frame_s1.ncols());
      // pertinence(p) = float(frame_s1(p)) / 255.f;
      // out(p) = distances;

    }

  }
  */

  template <typename V>
  void FFAST4(thread_info<CPU> ti,
                 kernel_image2d<i_float4> frame_color,
                 kernel_image2d<V> frame_s1,
                 kernel_image2d<V> frame_s2,
                 // kernel_image2d<dffast4> out,
                 kernel_image2d<i_float1> pertinence,
                 float grad_thresh)
  {

    point2d<int> p = thread_pos2d(ti);
    if (!frame_s1.has(p))//; || track(p).x == 0)
      return;


    if (p.row() < 6 || p.row() >= pertinence.domain().nrows() - 6 ||
        p.col() < 6 || p.col() >= pertinence.domain().ncols() - 6)
    {
      pertinence(p).x = 0.f;
      return;
    }

    gl01f pv;

    {
      float min_diff = 9999999.f;
      float max_single_diff = 0.f;
      pv = V(tex2D(flag<CPU>(), s1_tex, frame_s1, p));
      int sign = 0;
      for(int i = 0; i < 8; i++)
      {

        gl01f v1 = V(tex2D(flag<CPU>(), s1_tex, frame_s1,
			   p.col() + circle_r3_h[i][1],
			   p.row() + circle_r3_h[i][0]));

        gl01f v2 = V(tex2D(flag<CPU>(), s1_tex, frame_s1,
			   p.col() + circle_r3_h[(i+8)][1],
			   p.row() + circle_r3_h[(i+8)][0]));

        {
          float diff = pv -
            (v1 + v2) / 2.f;

          float adiff = fabs(diff);

          if (adiff < min_diff)
	  {
            min_diff = adiff;
            if (min_diff < 0.001f) break;
	  }



          if (max_single_diff < adiff) max_single_diff = adiff;

        }
      }

      pv = V(tex2D(flag<CPU>(), s2_tex, frame_s2, p.col()/2, p.row()/2));
      float min_diff_large = 9999999.f;
      float max_single_diff_large = 0.f;
      //int min_orientation_large;
      for(int i = 0; i < 8; i++)
      {

        gl01f v1 = V(tex2D(flag<CPU>(), s2_tex, frame_s2,
                         p.col()/2 + circle_r3_h[i][1],
			   p.row()/2 + circle_r3_h[i][0]));

        gl01f v2 = V(tex2D(flag<CPU>(), s2_tex, frame_s2,
                         p.col()/2 + circle_r3_h[(i+8)][1],
                         p.row()/2 + circle_r3_h[(i+8)][0]));

        {
          float diff = pv - (v1 + v2) / 2.f;
          float adiff = fabs(diff);

          if (adiff < min_diff_large)
          {
            min_diff_large = adiff;
            if (min_diff_large < 0.001f) break;
          }

          if (max_single_diff_large < adiff) max_single_diff_large = adiff;
        }

      }


      if (min_diff < min_diff_large)
      {
        min_diff = min_diff_large;
        max_single_diff = max_single_diff_large;
      }


      if (max_single_diff >= grad_thresh)
      {
        min_diff = min_diff / max_single_diff;
      }
      else
        min_diff = 0;

       pertinence(p) = min_diff;
      // pertinence(p) = p.col() / float(frame_s1.ncols());
      // pertinence(p) = float(frame_s1(p)) / 255.f;
      // out(p) = distances;

    }

  }

#define dffast4_to_color_sig(T)              \
kernel_image2d<dffast4> in,                  \
    kernel_image2d<i_float4> out,               \
    &dffast4_to_color<T>

  template <unsigned target>
  __host__ __device__  void dffast4_to_color(thread_info<target> ti,
                                                kernel_image2d<dffast4> in,
                                                kernel_image2d<i_float4> out)
  {
    point2d<int> p = thread_pos2d(ti);
    if (!in.has(p))
      return;

    i_float4 res;
    for (unsigned i = 0; i < 8; i+=2)
      //res[i/2] = (::abs(in(p).distances[i]) + ::abs(in(p).distances[i+1])) / (2*127.f);
      res[i/2] = (::abs(float(in(p).distances[i]) / 255.f) + ::abs(float(in(p).distances[i+1]) / 255.f)) / 2;
    res.w = 1.f;
    out(p) = res;
  }

  template <typename V, unsigned T>
  inline
  ffast4_feature<V, T>::ffast4_feature(const domain_t& d)
    : gl_frame_(d),
      blurred_s1_(d),
      blurred_s2_(d),
      tmp_(d),
      pertinence_(d),
      pertinence2_(d),
      f1_(d),
      f2_(d),
      ffast4_color_(d),
      color_blurred_(d),
      color_tmp_(d),
      grad_thresh_(0.03f),
      frame_cpt_(0)
  {
    f_prev_ = &f1_;
    f_ = &f2_;
#ifndef NO_CUDA
    cudaStreamCreate(&cuda_stream_);
#endif

  }

  template <typename V, unsigned T>
  inline
  void
  ffast4_feature<V, T>::update(const image2d_V& in, const image2d_V& in_s2)
  {
    frame_cpt_++;
    swap_buffers();
    dim3 dimblock(16, 16, 1);
    if (T == CPU)
      dimblock = dim3(in.ncols(), 2, 1);

    dim3 dimgrid = grid_dimension(in.domain(), dimblock);

    blurred_s1_ = in;
    blurred_s2_ = in_s2;

    //local_jet_static2_<0,0,1, 0,0,2, 6>::run(in, blurred_s1_, blurred_s2_, tmp_, pertinence2_);

    // if (!(frame_cpt_ % 5))
    {
      if (target == unsigned(GPU))
      {
	bindTexture2d(blurred_s1_, s1_tex);
	bindTexture2d(blurred_s2_, s2_tex);
      }

      pw_call<FFAST4_sig(target, V)>(flag<target>(), dimgrid, dimblock,
				     color_blurred_, blurred_s1_, blurred_s2_,
				     //*f_,
				     pertinence_, grad_thresh_);

      // filter_pertinence<i_float1><<<dimgrid, dimblock>>>
      //   (pertinence_, pertinence2_);
      // copy(pertinence2_, pertinence_);

      if (target == unsigned(GPU))
      {
	cudaUnbindTexture(s1_tex);
	cudaUnbindTexture(s2_tex);
	check_cuda_error();
      }
    }
  }

  template <typename V, unsigned T>
  inline
  const typename ffast4_feature<V, T>::image2d_f4&
  ffast4_feature<V, T>::feature_color() const
  {
    return ffast4_color_;
  }

  template <typename V, unsigned T>
  inline
  void
  ffast4_feature<V, T>::swap_buffers()
  {
    std::swap(f_prev_, f_);
  }

  template <typename V, unsigned T>
  inline
  void
  ffast4_feature<V, T>::set_detector_thresh(float t)
  {
    grad_thresh_ = t;
  }

  template <typename V, unsigned T>
  inline
  const typename ffast4_feature<V, T>::domain_t&
  ffast4_feature<V, T>::domain() const
  {
    return f1_.domain();
  }

  template <typename V, unsigned T>
  inline
  typename ffast4_feature<V, T>::image2d_D&
  ffast4_feature<V, T>::previous_frame()
  {
    return *f_prev_;
  }


  template <typename V, unsigned T>
  inline
  typename ffast4_feature<V, T>::image2d_D&
  ffast4_feature<V, T>::current_frame()
  {
    return *f_;
  }

  template <typename V, unsigned T>
  inline
  typename ffast4_feature<V, T>::image2d_V&
  ffast4_feature<V, T>::s1()
  {
    return blurred_s1_;
  }

  template <typename V, unsigned T>
  inline
  typename ffast4_feature<V, T>::image2d_V&
  ffast4_feature<V, T>::s2()
  {
    return blurred_s2_;
  }

  template <typename V, unsigned T>
  inline
  typename ffast4_feature<V, T>::image2d_f1&
  ffast4_feature<V, T>::pertinence()
  {
    return pertinence_;
  }


  template <typename V>
  template <unsigned target>
  __host__ __device__
  inline
  kernel_ffast4_feature<V>::kernel_ffast4_feature(ffast4_feature<V, target>& f)
    : pertinence_(f.pertinence()),
    s1_(f.s1()),
    s2_(f.s2())
  {
  }


  // inline
  // __host__ __device__ float
  // kernel_ffast4_feature<T>::distance(const point2d<int>& p_prev,
  //                               const point2d<int>& p_cur)
  // {
  //   return cuimg::distance_mean(f_prev_(p_prev), f_(p_cur));
  // }

  // inline
  // __host__ __device__ float
  // kernel_ffast4_feature<T>::distance(const dffast4& a,
  //                               const dffast4& b)
  // {
  //   return cuimg::distance_mean(a, b);
  // }


  template <typename V>
  inline
  __host__ __device__ float
  kernel_ffast4_feature<V>::distance_linear_s2(const dffast4& a,
                                const dffast4& b)
  {
    return cuimg::distance_mean_linear_s2(a, b);
  }


  // inline
  // __host__ __device__ float
  // kernel_ffast4_feature<V>::distance_linear(const point2d<int>& p_prev,
  //                                           const point2d<int>& p_cur)
  // {
  //   return cuimg::distance_mean_linear(f_(p_prev), f_(p_cur));
  // }

  template <typename V>
  inline
  __host__ __device__ float
  kernel_ffast4_feature<V>::distance_linear(const dffast4& a,
                                const dffast4& b)
  {
    return cuimg::distance_mean_linear(a, b);
  }


  template <typename V>
  inline
  __host__ __device__ dffast4
  kernel_ffast4_feature<V>::weighted_mean(const dffast4& a, float aw,
                                          const point2d<int>& n, float bw)
  {
    return new_state(n);
  }



  // union vector4f
  // {
  //   __m128i vi;
  //   unsigned char uc[16];
  //   unsigned short us[8];
  // };

  // template <typename V>
  // inline
  // __host__ __device__ float
  // kernel_ffast4_feature<V>::distance_linear(const dffast4& a,
  // 						const point2d<int>& n)
  // {
  //   // float d = 0;

  //   vector4f b;

  //   for(int i = 0; i < 8; i ++)
  //   {
  //     gl8u v = s1_(n.row() + circle_r3[i*2][0],
  // 		    n.col() + circle_r3[i*2][1]);
  //     b.uc[i] = v;
  //   }

  //   for(int i = 0; i < 8; i ++)
  //   {
  //     gl8u v = s2_(n.row() / 2 + circle_r3[i*2][0],
  //   		   n.col() / 2 + circle_r3[i*2][1]);
  //     b.uc[i+8] = v;
  //   }

  //   vector4f av;
  //   for(int i = 0; i < 8; i++)
  //     av.uc[i] = a[i];
  //   vector4f sum;
  //   sum.vi = _mm_sad_epu8(av.vi, b.vi);
  //   return (float(sum.us[0]) + float(sum.us[4])) / (255.f * 16.f);
  // }

  template <typename V>
  inline
  __host__ __device__ float
  kernel_ffast4_feature<V>::distance_linear(const dffast4& a,
  						const point2d<int>& n)
  {
    unsigned short d = 0;

    for(int i = 0; i < 4; i++)
    {
      gl8u v = s1_(n.row() + 2* c4[i][0],
  		   n.col() + 2* c4[i][1]);
      d += ::abs(v - a[i]);
    }

    for(int i = 0; i < 4; i++)
    {
      gl8u v = s2_(n.row() / 2 + 2* c4[i][0],
  		   n.col() / 2 + 2* c4[i][1]);
      d += ::abs(v - a[4+i]);
    }
    return d / (255.f * 16.f);

    // for(int i = 0; i < 16; i += 2)
    // {
    //   float v = s1_(n.row() + circle_r3[i][0],
    // 		   n.col() + circle_r3[i][1]).x * 255.f;
    //   d += fabs(v - a[i/2]);
    // }
    // for(int i = 0; i < 16; i += 2)
    // {
    //   float v = s2_(n.row() + 2 * circle_r3[i][0],
    // 		    n.col() + 2 * circle_r3[i][1]).x * 255.f;
    //   d += fabs(v - a[8+i/2]);
    // }
    // return d / (255.f * 16.f);

    // return cuimg::distance_mean_linear(a, new_state(n));
  }

  template <typename V>
  inline
  __host__ __device__ float
  kernel_ffast4_feature<V>::distance_s2(const dffast4& a,
					const dffast4& b)
  {
    return cuimg::distance_mean_s2(a, b);
  }

  // inline __host__ __device__
  // kernel_image2d<dffast4>&
  // kernel_ffast4_feature<V>::previous_frame()
  // {
  //   return f_prev_;
  // }

  // inline __host__ __device__
  // kernel_image2d<dffast4>&
  // kernel_ffast4_feature<V>::current_frame()
  // {
  //   return f_;
  // }


  template <typename V>
  inline __host__ __device__
  kernel_image2d<V>&
  kernel_ffast4_feature<V>::s1()
  {
    return s1_;
  }

  template <typename V>
  inline __host__ __device__
  kernel_image2d<V>&
  kernel_ffast4_feature<V>::s2()
  {
    return s2_;
  }

  template <typename V>
  inline __host__ __device__
  kernel_image2d<i_float1>&
  kernel_ffast4_feature<V>::pertinence()
  {
    return pertinence_;
  }

  template <typename V>
  inline
  __host__ __device__ dffast4
  kernel_ffast4_feature<V>::new_state(const point2d<int>& n)
  {
    dffast4 b;
    for(int i = 0; i < 4; i++)
      b[i] = s1_(n.row() + 2 * c4[i][0],
    		   n.col() + 2 * c4[i][1]);
    for(int i = 0; i < 4; i++)
      b[i+4] = s2_(n.row() / 2 + 2 * c4[i][0],
    		     n.col() / 2 + 2 * c4[i][1]);
    return b;
  }

  template <typename V, unsigned T>
  inline
  void
  ffast4_feature<V, T>::display() const
  {
#ifdef WITH_DISPLAY
    dim3 dimblock(16, 16, 1);
    dim3 dimgrid = grid_dimension(pertinence_.domain(), dimblock);

    // pw_call<dffast4_to_color_sig(target) >(flag<target>(), dimgrid, dimblock, *f_, ffast4_color_);

    ImageView("test") <<= dg::dl() - gl_frame_ - blurred_s1_ - blurred_s2_ - pertinence_ - ffast4_color_;
#endif
  }
}

#endif // ! CUIMG_FFAST4_FEATURE_HPP_
