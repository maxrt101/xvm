#ifndef _XVM_DEVICES_VIDEO_H_
#define _XVM_DEVICES_VIDEO_H_ 1

#include <xvm/bus.h>
#include <SDL2/SDL.h>


#define XVM_BUS_DEV_VIDEO_NAME "video"

#define XVM_VIDEO_GLOBAL_STATUS_REGISTER 0
#define XVM_VIDEO_STATE_REGISTER         1
#define XVM_VIDEO_MODE_REGISTER          2
#define XVM_VIDEO_CMD_REGISTER           3
#define XVM_VIDEO_UPDATE_REGISTER        4
#define XVM_VIDEO_COLOR_R_REGISTER       5
#define XVM_VIDEO_COLOR_G_REGISTER       6
#define XVM_VIDEO_COLOR_B_REGISTER       7
#define XVM_VIDEO_COLOR_A_REGISTER       8
#define XVM_VIDEO_MEM_OFFSET             256


namespace xvm {
namespace bus {
namespace device {

enum class VideoMode {
  NONE  = 0,
  PIXEL = 1,
  TEXT  = 2,
};

enum class VideoStatus {
  UNINITIALIZED = 0,
  INITIALIZED   = 1,
};

enum class VideoState {
  NONE = 0
};

class Video : public Device {
 private:
  size_t m_fontSize = 14;
  size_t m_screenWidth = 0;
  size_t m_screenHeight = 0;
  size_t m_baseAddr = 0;
  SDL_Color m_color {255, 255, 255, 255};
  VideoMode m_mode = VideoMode::NONE;
  VideoStatus m_status = VideoStatus::UNINITIALIZED;
  VideoState m_state = VideoState::NONE;
  SDL_Window* m_window = nullptr;
  SDL_Renderer* m_renderer = nullptr;

 public:
  Video();
  ~Video();

  void initialize(size_t width, size_t height, size_t base);
  void cleanup();

  void update();

  void setMode(VideoMode mode);
  VideoMode getMode() const;
  uint8_t read(size_t address) override;
  void write(size_t address, uint8_t value) override;
};

} /* namespace device */
} /* namespace bus */
} /* namespace xvm */

#endif