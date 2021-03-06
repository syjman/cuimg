#include <iostream>
#include <vector>

#include <cuimg/profiler.h>
#include <cuimg/video_capture.h>
#include <cuimg/improved_builtin.h>
#include <cuimg/target.h>

#include <cuimg/copy.h>
#include <cuimg/dsl/all.h>
#include <cuimg/gpu/device_image2d.h>
#include <cuimg/cpu/host_image2d.h>

#include <cuimg/tracking2/tracker.h>


using namespace cuimg;

// trajectory store a short term trajectory.
struct trajectory
{
  trajectory() : alive(true) {}
  trajectory(i_int2 pos) : alive(true) { history.push_back(pos); }

  void move(trajectory&& t)
  {
    history.swap(t.history);
    alive = t.alive;
  }

  std::deque<i_int2> history;
  bool alive;
};


// Update a trajectory when a particle moves.
template <typename TR>
void update_trajectories(std::vector<trajectory>& v, TR& pset)
{
  const auto& parts = pset.dense_particles();
  for(unsigned i = 0; i < v.size(); i++)
    v[i].alive = false;
  for(unsigned i = 0; i < v.size(); i++)
    if (parts[i].age > 0)
    {
      assert(parts[i].age != 1 || v[i].history.empty());
      v[i].history.push_back(parts[i].pos);
      v[i].alive = true;
      if (v[i].history.size() > 20) v[i].history.pop_front();
    }
    else
    {
      v[i].history.clear();
    }
}

int main(int argc, char* argv[])
{
  cv::VideoCapture video;

  if (argc == 4)
  {
    video.open(argv[1]);
  }
  else
  {
    std::cout << "Usage: ./tracking_qt video_file nscales detector_threshold" << std::endl;
    return -1;
  }

  if(!video.isOpened())
  {
    std::cout << "Cannot open " << argv[1] << std::endl;
    return -1;
  }

  int NSCALES = atoi(argv[2]);
  if (NSCALES <= 0 or NSCALES >= 10)
  {
    std::cout << "NSCALE should be > 0 and < 10, got " << argv[2] << std::endl;
    return -1;
  }

  int detector_threshold = atoi(argv[3]);

  obox2d domain(video.get(CV_CAP_PROP_FRAME_HEIGHT), video.get(CV_CAP_PROP_FRAME_WIDTH));
  host_image2d<gl8u> frame_gl(domain);

  // Tracker definition
  typedef tracker<tracking_strategies::bc2s_fast_gradient_cpu> T1;
  T1 tr1(domain, NSCALES);

  // Tracker settings
  tr1.strategy().set_detector_frequency(1);
  tr1.strategy().set_filtering_frequency(1);
  for (unsigned s = 0; s < NSCALES; s++)
    tr1.scale(s).strategy().detector().set_n(9).set_fast_threshold(detector_threshold);

  // Record trajectories at each scales.
  std::vector<std::vector<trajectory> > trajectories(NSCALES);

  cv::Mat input_;
  while (video.read(input_)) // For each frame
  {
    host_image2d<i_uchar3> frame(input_);
    frame_gl = get_x(frame); // Basic Gray level conversion.
    tr1.run(frame_gl);

    for (unsigned s = 0; s < NSCALES; s++)
    {
      // Sync trajectories buffer with particles
      tr1.scale(s).pset().sync_attributes(trajectories[s], trajectory());
      // Update trajectories.
      update_trajectories(trajectories[s], tr1.scale(s).pset());
    }
  }

  return 0;
}
