#ifndef CUIMG_MULTI_SCALE_TRACKER_HPP_
# define CUIMG_MULTI_SCALE_TRACKER_HPP_

# include <cuimg/improved_builtin.h>
# include <cuimg/tracking/multi_scale_tracker.h>
# include <cuimg/gpu/mipmap.h>
# include <cuimg/pw_call.h>
# include <dige/recorder.h>

namespace cuimg
{

  template <typename F, template <class>  class SA_>
  inline
  multi_scale_tracker<F, SA_>::multi_scale_tracker(const D& d)
    : frame_uc3_(d),
      frame_(d),
      // traj_tracer_(d),
      dummy_matches_(d),
      mvt_detector_thread_(d)
  {
    pyramid_ = allocate_mipmap(i_float1(), frame_, PS);
    pyramid_tmp1_ = allocate_mipmap(i_float1(), frame_, PS);
    pyramid_tmp2_ = allocate_mipmap(i_float1(), frame_, PS);

    pyramid_display1_ = allocate_mipmap(i_float4(), frame_, PS);
    pyramid_display2_ = allocate_mipmap(i_float4(), frame_, PS);
    pyramid_speed_ = allocate_mipmap(i_float4(), frame_, PS);
    for (unsigned i = 0; i < pyramid_.size(); i++)
    {
      feature_.push_back(new F(pyramid_[i].domain()));
      matcher_.push_back(new SA(pyramid_[i].domain()));
    }

    fill(dummy_matches_, i_short2(-1, -1));

#ifdef WITH_DISPLAY
    //  ImageView("video", d.ncols() * 2, d.nrows() * 2);
#endif

  }

  template <typename F, template <class>  class SA_>
  multi_scale_tracker<F, SA_>::~multi_scale_tracker()
  {
    for (unsigned i = 0; i < pyramid_.size(); i++)
    {
      delete feature_[i];
      delete matcher_[i];
    }
  }

  template <typename F, template <class>  class SA_>
  inline
  const typename multi_scale_tracker<F, SA_>::D&
  multi_scale_tracker<F, SA_>::domain() const
  {
    return frame_.domain();
  }


  template <typename V>
  __host__ __device__ void plot_c4(kernel_image2d<V>& out, point2d<int>& p, const V& value)
  {
    for_all_in_static_neighb2d(p, n, c5) if (out.has(n))
      out(n) = value;
    out(p) = i_float4(0.f, 0.f, 0.f, 1.f);
  }

  template <unsigned target, typename P>
  __host__ __device__ void draw_particles(thread_info<target> ti,
                                          kernel_image2d<i_float1> frame,
                                          kernel_image2d<P> pts,
                                          kernel_image2d<i_float4> out,
                                          int age_filter)
  {
    point2d<int> p = thread_pos2d(ti);
    if (!frame.has(p))
      return;

    //if (pts(p).age < age_filter) out(p) = frame(p);
    if (pts(p).age >= age_filter) //else out(p) = i_float4(1.f, 0, 0, 1.f);
      plot_c4(out, p, i_float4(1.f, 0.f, 0, 1.f));
  }

#define draw_particles_sig(T, P) kernel_image2d<i_float1>, kernel_image2d<P>, \
    kernel_image2d<i_float4>, int, &draw_particles<T, P>

  template <unsigned target, typename P>
  __host__ __device__ void draw_speed(thread_info<target> ti, kernel_image2d<P> track, kernel_image2d<i_float4> out)
  {
    point2d<int> p = thread_pos2d(ti);
    if (!track.has(p))
      return;

    if (!track(p).age)
    {
       out(p) = i_float4(0, 0, 0, 1.f);
       return;
    }

    i_float2 speed(track(p).speed);
    float module = norml2(speed);

    float R = ::abs(2 * track(p).speed.x + module) / 14.f;
    float G = ::abs(-track(p).speed.x - track(p).speed.y + module) / 14.f;
    float B = ::abs(2 * track(p).speed.y + module) / 14.f;

    R = R < 0.f ? 0.f : R;
    G = G < 0.f ? 0.f : G;
    B = B < 0.f ? 0.f : B;
    out(p) = i_float4(R, G, B, 1.f);
  }

#define draw_speed_sig(T, P) kernel_image2d<P>, kernel_image2d<i_float4>, &draw_speed<T, P>

  template <unsigned target, typename P>
  __host__ __device__ void of_reconstruction(thread_info<target> ti, kernel_image2d<i_float4> speed, kernel_image2d<i_float1> in, kernel_image2d<i_float4> out)
  {
    point2d<int> p = thread_pos2d(ti);
    if (!speed.has(p))
      return;

    // if (speed(p) != i_float4(0, 0, 0, 1.f))
    // {
    //   out(p) = speed(p);
    //   return;
    // }

    i_float4 res = zero();
    int cpt = 0;
    for_all_in_static_neighb2d(p, n, c49) if (speed.has(n) && speed(n) != i_float4(0, 0, 0, 1.f))
    {
       if (norml2(in(p) - in(n)) < (30.f / 256.f))
       {
          res += speed(n);
          cpt++;
       }
    }

    if (cpt >= 2)
      out(p) = res / cpt;
    else
      out(p) = i_float4(0, 0, 0, 1.f);
  }

#define of_reconstruction_sig(T, P) kernel_image2d<i_float4>, kernel_image2d<i_float1>, \
            kernel_image2d<i_float4>, &of_reconstruction<T, P>

  template <unsigned target, typename P>
  __host__ __device__ void sub_global_mvt(thread_info<target> ti, const i_short2* particles_vec,
                                          unsigned n_particles, kernel_image2d<P> particles, i_short2 mvt)
  {
    unsigned threadid = ti.blockIdx.x * ti.blockDim.x + ti.threadIdx.x;
    if (threadid >= n_particles) return;
    point2d<int> p = particles_vec[threadid];

    // point2d<int> p = thread_pos2d();
    if (!particles.has(p))
      return;

    i_float2 speed = particles(p).speed;
    if (particles(p).age > 2)
    {
      //particles(p).acceleration -= mvt;
      speed -= mvt * 3.f / 4.f;
    }
    else
      speed -= mvt;

    particles(p).speed = speed;
  }

#define sub_global_mvt_sig(T, P) const i_short2*, unsigned,      \
    kernel_image2d<P>, i_short2, &sub_global_mvt<T, P>

  template <unsigned target, typename P>
  __host__ __device__ void draw_particles2(thread_info<target> ti, kernel_image2d<P> pts,
                                          kernel_image2d<i_float4> out,
                                          int age_filter)
  {
    point2d<int> p = thread_pos2d(ti);
    if (!out.has(p))
      return;

    if (pts(p).age < age_filter) out(p) = i_float4(0.f, 0.f, 0.f, 1.f);
    // else plot_c4(out, p, i_float4(1.f, 0, 0, 1.f));
    else out(p) = i_float4(1.f, 0.f, 0, 1.f);
  }

#define draw_particles2_sig(T, P) kernel_image2d<P>, kernel_image2d<i_float4>, int, &draw_particles2<T, P>

  template <unsigned target, typename P>
  __host__ __device__ void draw_particle_pertinence(thread_info<target> ti,
                                                    kernel_image2d<i_float1> pertinence,
                                                    kernel_image2d<P> pts,
                                                    kernel_image2d<i_float4> out,
                                                    int age_filter)
  {
    point2d<int> p = thread_pos2d(ti);
    if (!out.has(p))
      return;

    if (pts(p).age > age_filter) out(p) = i_float4(1.f, 0.f, 0.f, 1.f);
    else
      out(p) = i_float4(pertinence(p), pertinence(p), pertinence(p), 1.f);
  }

#define draw_particle_pertinence_sig(T, P) kernel_image2d<i_float1>, kernel_image2d<P>, \
    kernel_image2d<i_float4>, int, &draw_particle_pertinence<T, P>


extern "C" {
  __inline__ uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ (      // serialize
    "xorl %%eax,%%eax \n        cpuid"
    ::: "%rax", "%rbx", "%rcx", "%rdx");
    /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t)hi << 32 | lo;
  }
}

#define BENCH_CPP_START(M)                      \
  unsigned long long cycles_##M = rdtsc();

#define BENCH_CPP_END(M, NP)                                            \
  cycles_##M = rdtsc() - cycles_##M;                                    \
  typedef unsigned long long ull;                                       \
  std::cout << "[" << #M << "]\tCycles per pixel: " << std::fixed << float((double(cycles_##M) / (NP))) << std::endl;

  template <typename I, typename O>
  void to_graylevelf(const Image2d<I>& in_, Image2d<O>& out_)
  {
    const I& in = exact(in_);
    O& out = exact(out_);

#pragma omp parallel for schedule(static, 8)
    for (unsigned r = 0; r < in.nrows(); r++)
      for (unsigned c = 0; c < in.ncols(); c++)
      {
        i_uchar3 x = in(r, c);
        out(r, c) = (int(x.x) + x.y + x.z) / (3 * 255.f);
      }
  }


  template <typename I, typename J>
  void prepare_input_frame(const flag<GPU>&, const host_image2d<i_uchar3>& in, I& frame, J& tmp_uc3)
  {
      copy(in, tmp_uc3);
      frame = (get_x(tmp_uc3) + get_y(tmp_uc3) + get_z(tmp_uc3)) / (3.f * 255.f);
  }

  template <typename I, typename J>
  void prepare_input_frame(const flag<CPU>&, const host_image2d<i_uchar3>& in, I& frame, J&)
  {
      to_graylevelf(in, frame);
  }

  template <typename F, template <class>  class SA_>
  inline
  void multi_scale_tracker<F, SA_>::update(const host_image2d<i_uchar3>& in)
  {
    prepare_input_frame(flag<target>(), in, frame_, frame_uc3_);

    update_mipmap(frame_, pyramid_, pyramid_tmp1_, pyramid_tmp2_, PS);


    i_short2 mvt(0, 0);

   // ImageView("frame test") <<= dg::dl() - pyramid_[4] - pyramid_[3] - pyramid_[2] - pyramid_[1] - pyramid_[0] +
   //                                         pyramid_tmp1_[4] - pyramid_tmp1_[3] - pyramid_tmp1_[2] - pyramid_tmp1_[1] - pyramid_tmp1_[0] +
   //                                         pyramid_tmp2_[4] - pyramid_tmp2_[3] - pyramid_tmp2_[2] - pyramid_tmp2_[1] - pyramid_tmp2_[0];

    for (int l = pyramid_.size() - 2; l >= 0; l--)
    {
      mvt *= 2;

      feature_[l]->update(pyramid_[l], pyramid_[l + 1]);
      if (l != pyramid_.size() - 1)
        matcher_[l]->update(*(feature_[l]), mvt_detector_thread_, matcher_[l+1]->matches());
      else
        matcher_[l]->update(*(feature_[l]), mvt_detector_thread_, dummy_matches_);

      // if (l >= 0)
      // {
      //   mvt_detector_thread_.update(*(matcher_[l]), l);
      // }

      // dim3 dimblock(128, 1);
      // dim3 reduced_dimgrid(1 + matcher_[l]->n_particles() / (dimblock.x * dimblock.y), dimblock.y, 1);

      // pw_call<sub_global_mvt_sig(target, P)>(flag<target>(), reduced_dimgrid, dimblock,
      //                                thrust::raw_pointer_cast(&matcher_[l]->particle_positions()[0]),
      //                                matcher_[l]->n_particles(), matcher_[l]->particles(), mvt);

      //if (l == pyramid_.size() - 3) mvt_detector.display();
    }

#ifdef WITH_DISPLAY
    display();
#endif

  }


  template <typename F, template <class>  class SA_>
  inline
  void multi_scale_tracker<F, SA_>::display()
  {
#ifdef WITH_DISPLAY

    flag<target> f;

//#define TD 0
    unsigned TD = Slider("display_scale").value();
    if (TD >= PS) TD = PS - 1;
    //traj_tracer_.update(matcher_[TD]->matches(), matcher_[TD]->particles());

    D d = matcher_[TD]->particles().domain();
    dim3 db(16, 16);
    dim3 dg = dim3(grid_dimension(d, db));

    feature_[TD]->display();
    matcher_[TD]->display();
    // mvt_detector_thread_.mvt_detector().display();
    //traj_tracer_.display("trajectories", pyramid_[TD]);

    pyramid_display1_[TD] = aggregate<float>::run(get_x(pyramid_[TD]), get_x(pyramid_[TD]), get_x(pyramid_[TD]), 1.f);

    pw_call<draw_particles_sig(target, P)>(f,dg,db,pyramid_[TD], matcher_[TD]->particles(), pyramid_display1_[TD], Slider("age_filter").value());

    pw_call<draw_particles2_sig(target, P)>(f, dg,db, matcher_[TD]->particles(), pyramid_display2_[TD], Slider("age_filter").value());

    image2d_f4 particle_pertinence(matcher_[TD]->particles().domain());
    image2d_f4 of(matcher_[TD]->particles().domain());
    pw_call<draw_particle_pertinence_sig(target, P)>(f,dg,db,feature_[TD]->pertinence(), matcher_[TD]->particles(),
                                             particle_pertinence, Slider("age_filter").value());

    pw_call<draw_speed_sig(target, P)>
      (f, dg,db, kernel_image2d<P>(matcher_[TD]->particles()),
       kernel_image2d<i_float4>(pyramid_speed_[TD]));

    pw_call<of_reconstruction_sig(target, P)>(f,dg,db,pyramid_speed_[TD], pyramid_[TD], of);

    ImageView("frame") <<= dg::dl() - frame_ - pyramid_display1_[TD] - pyramid_display2_[TD] - pyramid_speed_[TD] + of - particle_pertinence;

    // dg::record("views.avi") <<= ImageView("video") <<= dg::dl() - pyramid_display1_[TD] - feature_[TD]->pertinence() +
    //   traj_tracer_.straj() - of;
#endif
  }

}

#endif // !CUIMG_MULTI_SCALE_TRACKER_HPP_

