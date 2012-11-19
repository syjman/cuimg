#ifndef CUIMG_PARTICLE_CONTAINER_HP_
# define CUIMG_PARTICLE_CONTAINER_HP_

# include <cuimg/memset.h>
# include <cuimg/tracking2/compact_particles_image.h>
# include <cuimg/mt_apply.h>

namespace cuimg
{

  // ################### Particle container methods ############
  // ###########################################################

  template <typename F,
	    template <class> class I>
  particle_container<F, I>::particle_container(const obox2d& d)
    : sparse_buffer_(d),
      matches_(d)
  {
    particles_vec_.reserve((d.nrows() * d.ncols()) / 10);
    features_vec_.reserve((d.nrows() * d.ncols()) / 10);
  }

  template <typename F,
	    template <class> class I>
  typename particle_container<F, I>::V&
  particle_container<F, I>::dense_particles()
  {
    return particles_vec_;
  }

  template <typename F,
	    template <class> class I>
  I<unsigned short>&
  particle_container<F, I>::sparse_particles()
  {
    return sparse_buffer_;
  }


  template <typename F,
	    template <class> class I>
  const typename particle_container<F, I>::V&
  particle_container<F, I>::dense_particles() const
  {
    return particles_vec_;
  }

  template <typename F,
	    template <class> class I>
  const I<unsigned short>&
  particle_container<F, I>::sparse_particles() const
  {
    return sparse_buffer_;
  }

  template <typename F,
	    template <class> class I>
  const typename particle_container<F, I>::FV&
  particle_container<F, I>::features()
  {
    return features_vec_;
  }

  template <typename F,
	    template <class> class I>
  const particle& 
  particle_container<F, I>::operator()(i_short2 p) const
  {
    assert(has(p));
    return particles_vec_[sparse_buffer_(p)];
  }

  template <typename F,
	    template <class> class I>
  particle& 
  particle_container<F, I>::operator()(i_short2 p)
  {
    assert(has(p));
    return particles_vec_[sparse_buffer_(p)];
  }

  template <typename F,
	    template <class> class I>
  const particle& 
  particle_container<F, I>::operator[](unsigned i) const
  {
    return particles_vec_[i];
  }

  template <typename F,
	    template <class> class I>
  particle& 
  particle_container<F, I>::operator[](unsigned i)
  {
    return particles_vec_[i];
  }

  template <typename F,
	    template <class> class I>
  const typename particle_container<F, I>::feature_type&
  particle_container<F, I>::feature_at(i_short2 p)
  {
    assert(has(p));
    return features_vec_[sparse_buffer_(p)];
  }

  template <typename F,
	    template <class> class I>
  const I<i_short2>&
  particle_container<F, I>::matches()
  {
    return matches_;
  }

  template <typename F,
	    template <class> class I>
  void
  particle_container<F, I>::before_matching()
  {
    // Reset matches, new particles and features.
    memset(matches_, 0);
    memset(sparse_buffer_, 255);
  }

  template <typename F,
	    template <class> class I>
  void
  particle_container<F, I>::after_matching()
  {
  }

  template <typename F,
	    template <class> class I>
  void
  particle_container<F, I>::after_new_particles()
  {
    compact();
  }

  template <typename F,
	    template <class> class I>
  void
  particle_container<F, I>::compact()
  {
    SCOPE_PROF(compation);
    auto pts_it = particles_vec_.begin();
    auto feat_it = features_vec_.begin();
    auto pts_res = particles_vec_.begin();
    auto feat_res = features_vec_.begin();

    for (;pts_it != particles_vec_.end();)
    {
      if (pts_it->age != 0)
      {
	*pts_res++ = *pts_it;
	*feat_res++ = *feat_it;
	sparse_buffer_(pts_it->pos) = pts_res - particles_vec_.begin() - 1;
	assert(particles_vec_[sparse_buffer_(pts_it->pos)].pos == pts_it->pos);
      }

      pts_it++;
      feat_it++;
    }

    // std::cout << particles_vec_.size() << std::endl;
    // std::cout << pts_res - particles_vec_.begin() << std::endl;
    particles_vec_.resize(pts_res - particles_vec_.begin());
    features_vec_.resize(feat_res - features_vec_.begin());

    //compact_particles_image(*cur_particles_img_, particles_vec_);
  }

  template <typename F,
	    template <class> class I>
  void
  particle_container<F, I>::add(i_int2 p, const typename F::value_type& feature)
  {
    particle pt;
    pt.age = 1;
    pt.speed = i_int2(0,0);
    pt.pos = p;

    sparse_buffer_(p) = particles_vec_.size();
    particles_vec_.push_back(pt);
    features_vec_.push_back(feature);
  }


  template <typename F,
	    template <class> class I>
  void
  particle_container<F, I>::remove(int i)
  {
    particle& p = particles_vec_[i];
    p.age = 0;
  }

  template <typename F,
	    template <class> class I>
  void
  particle_container<F, I>::remove(const i_short2& pos)
  {
    particle& p = particles_vec_[sparse_buffer_(p.pos)];
    p.age = 0;
  }

  template <typename F,
	    template <class> class I>
  void
  particle_container<F, I>::move(unsigned i, i_int2 dst)
  {
    particle& p = particles_vec_[i];
    i_int2 src = p.pos;
    p.age++;
    i_int2 new_speed = dst - src;
    if (p.age > 1)
      p.acceleration = new_speed - p.speed;
    else
      p.acceleration = i_int2(0,0);
    p.speed = new_speed;
    p.pos = dst;

    sparse_buffer_(dst) = i;
    matches_(src) = dst;
  }

  template <typename F,
	    template <class> class I>
  bool
  particle_container<F, I>::has(i_int2 p) const
  {
    unsigned short r = -1;
    return sparse_buffer_(p) != r;
  }

  template <typename F,
	    template <class> class I>
  bool
  particle_container<F, I>::is_coherent()
  {
    // for (unsigned i = 0; i < dense_particles().size(); i++)
    // {
    //   i_short2 pos = dense_particles()[i].pos;
    //   if (dense_particles()[i].age > 0 && sparse_particles()(pos).vpos != i)
    //   {
    // 	std::cout << "i: " << i << "vpos: " << sparse_particles()(pos).vpos  << " pos:" << pos << " vpos.pos:" << 
    // 		  dense_particles()[sparse_particles()(pos).vpos].pos << std::endl;
    // 	std::cout << "p.age: " << dense_particles()[i].age << 
    // 	             "sp.age: " << sparse_particles()(pos).age << std::endl;

    // 	return false;
    //   }
    // }

    // bool ok = true;
    // mt_apply2d(sparse_particles().domain(), [this, &ok] (i_int2 p)
    // 	       {
    // 		 if (this->sparse_particles()(p).age == 0) return;
    // 		 int vpos = this->sparse_particles()(p).vpos;
    // 		 if (this->dense_particles()[vpos].pos != p)
    // 		 {
    // 		   std::cout << "vpos: " << vpos << " " << this->dense_particles()[vpos].pos << " != " <<  p << std::endl;
    // 		   ok = false;
    // 		 }
    // 	       });
    return true;
  }

}

#endif