#include <xvm/devices/video.h>
#include <xvm/log.h>

#include <cstdlib>

xvm::bus::device::Video::Video() : Device(XVM_BUS_DEV_VIDEO_NAME) {}

xvm::bus::device::Video::~Video() {}

void xvm::bus::device::Video::initialize(size_t width, size_t height, size_t base) {
  if (m_status == VideoStatus::INITIALIZED) return;

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    sdlFatal("SDL_Init failed");
    exit(1);
  }

  m_screenWidth = width;
  m_screenHeight = height;
  m_baseAddr = base;

  m_window = SDL_CreateWindow(
    "xvm",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    width,
    height,
    SDL_WINDOW_SHOWN
  );

  m_renderer = SDL_CreateRenderer(m_window, -1, 0);
  if (m_renderer == NULL) {
    sdlFatal("Renderer creation failed");
    exit(1);
  }

  SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

  m_status = VideoStatus::INITIALIZED;
}

void xvm::bus::device::Video::cleanup() {
  SDL_DestroyRenderer(m_renderer);
  SDL_DestroyWindow(m_window);
  SDL_Quit();
}

void xvm::bus::device::Video::update() {
  if (m_status != VideoStatus::INITIALIZED) return;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT: {
        cleanup();
        exit(1);
      }
    }
  }

  SDL_RenderPresent(m_renderer);

  SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
  SDL_RenderClear(m_renderer);
  SDL_SetRenderDrawColor(m_renderer, m_color.r, m_color.g, m_color.b, m_color.a);
}

void xvm::bus::device::Video::setMode(VideoMode mode) {
  if (m_mode != VideoMode::NONE) {}

  m_mode = mode;
}

xvm::bus::device::VideoMode xvm::bus::device::Video::getMode() const {
  return m_mode;
}

uint8_t xvm::bus::device::Video::read(size_t address) {
  if (address == m_baseAddr + XVM_VIDEO_GLOBAL_STATUS_REGISTER) {
    return (uint8_t)m_status;
  } else if (address == m_baseAddr + XVM_VIDEO_STATE_REGISTER) {
    return (uint8_t)m_state;
  } else if (address == m_baseAddr + XVM_VIDEO_MODE_REGISTER) {
    return (uint8_t)m_mode;
  } else if (address == m_baseAddr + XVM_VIDEO_CMD_REGISTER) {
    return 0;
  } else if (address == m_baseAddr + XVM_VIDEO_UPDATE_REGISTER) {
    return 0;
  } else if (address == m_baseAddr + XVM_VIDEO_COLOR_R_REGISTER) {
    return m_color.r;
  } else if (address == m_baseAddr + XVM_VIDEO_COLOR_G_REGISTER) {
    return m_color.g;
  } else if (address == m_baseAddr + XVM_VIDEO_COLOR_B_REGISTER) {
    return m_color.b;
  } else if (address == m_baseAddr + XVM_VIDEO_COLOR_A_REGISTER) {
    return m_color.a;
  } else if (address >= m_baseAddr + XVM_VIDEO_MEM_OFFSET) {
    return 0;
  } else {
    return 0;
  }
}

void xvm::bus::device::Video::write(size_t address, uint8_t value) {
  if (address == m_baseAddr + XVM_VIDEO_GLOBAL_STATUS_REGISTER) {
    warning("video: attempted write into status register");
  } else if (address == m_baseAddr + XVM_VIDEO_STATE_REGISTER) {
    warning("video: attempted write into state register");
  } else if (address == m_baseAddr + XVM_VIDEO_MODE_REGISTER) {
    setMode((VideoMode)value);
  } else if (address == m_baseAddr + XVM_VIDEO_CMD_REGISTER) {
    warning("video: attemped wrtie into cmd register");
  } else if (address == m_baseAddr + XVM_VIDEO_UPDATE_REGISTER) {
    if (value == 1) {
      update();
    }
  } else if (address == m_baseAddr + XVM_VIDEO_COLOR_R_REGISTER) {
    m_color.r = value;
    SDL_SetRenderDrawColor(m_renderer, m_color.r, m_color.g, m_color.b, m_color.a);
  } else if (address == m_baseAddr + XVM_VIDEO_COLOR_G_REGISTER) {
    m_color.g = value;
    SDL_SetRenderDrawColor(m_renderer, m_color.r, m_color.g, m_color.b, m_color.a);
  } else if (address == m_baseAddr + XVM_VIDEO_COLOR_B_REGISTER) {
    m_color.b = value;
    SDL_SetRenderDrawColor(m_renderer, m_color.r, m_color.g, m_color.b, m_color.a);
  } else if (address == m_baseAddr + XVM_VIDEO_COLOR_A_REGISTER) {
    m_color.a = value;
    SDL_SetRenderDrawColor(m_renderer, m_color.r, m_color.g, m_color.b, m_color.a);
  } else if (address >= m_baseAddr + XVM_VIDEO_MEM_OFFSET) {
    if (m_mode == VideoMode::PIXEL) {
      int offset = (address - m_baseAddr - XVM_VIDEO_MEM_OFFSET);
      if (value == 1) {
        SDL_RenderDrawPoint(m_renderer, offset % m_screenWidth + 1, offset / m_screenWidth + 1);
      } else if (value == 0) {
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderDrawPoint(m_renderer, offset % m_screenWidth + 1, offset / m_screenWidth + 1);
        SDL_SetRenderDrawColor(m_renderer, m_color.r, m_color.g, m_color.b, m_color.a);
      }
    } else if (m_mode == VideoMode::TEXT) {
      warning("video: text mode unimplemented");
    }
  }
}
