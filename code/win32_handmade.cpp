#include <cmath>
#include <cstdint>
#include <stdint.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

struct Vec3 {
    double x;
    double y;
    double z;

    Vec3(double ix, double iy, double iz) {
        x = ix;
        y = iy;
        z = iz;
    }

    Vec3 operator+(Vec3 vec) { return Vec3(x + vec.x, y + vec.y, z + vec.z); }
    Vec3 operator-(Vec3 vec) { return Vec3(x - vec.x, y - vec.y, z - vec.z); }
    Vec3 operator*(double t) { return Vec3(x * t, y * t, z * t); }

    Vec3 operator*=(double t) {
        x *= t;
        y *= t;
        z *= t;
        return Vec3(x, y, z);
    }
    Vec3 operator/(double t) { return Vec3(x / t, y / t, z / t); }
    double length() { return sqrt(x * x + y * y + z * z); }
};

Vec3 operator*(double t, Vec3 vec) { return vec * t; }

struct Point3 {
    double x;
    double y;
    double z;

    Point3(double ix, double iy, double iz) {
        x = ix;
        y = iy;
        z = iz;
    }
    Point3(Vec3 vec) {
        x = vec.x;
        y = vec.y;
        z = vec.z;
    }
    Vec3 operator-(Vec3 vec) { return Vec3(x - vec.x, y - vec.y, z - vec.z); }
    Vec3 operator-(Point3 p) { return Vec3(x - p.x, y - p.y, z - p.z); }
    Point3 operator+(Vec3 vec) {
        return Point3(x + vec.x, y + vec.y, z + vec.z);
    }
};

struct Ray {
    Point3 origin;
    Vec3 direction;

    Ray(Point3 origin, Vec3 direction) : origin(origin), direction(direction) {}

    Point3 at(double t) { return origin + t * direction; }
};

struct Color {
    double r;
    double g;
    double b;

    Color(double ir, double ig, double ib) {
        r = ir;
        g = ig;
        b = ib;
    }

    uint32_t toPixel() {
        uint8_t rbyte = uint8_t(255.999 * r);
        uint8_t gbyte = uint8_t(255.999 * g);
        uint8_t bbyte = uint8_t(255.999 * b);
        return (rbyte << 16) | (gbyte << 8) | bbyte;
    }
    Color operator*(double t) { return Color(r * t, g * t, b * t); }
    Color operator+(Color c) { return Color(r + c.r, g + c.g, b + c.b); }
};

Color operator*(double t, Color c) { return Color(c.r * t, c.g * t, c.b * t); }

Vec3 unit_vector(Vec3 vec) { return vec / vec.length(); }

double dot(Vec3 u, Vec3 v) { return u.x * v.x + u.y * v.y + u.z * v.z; }

double hit_sphere(Point3 center, double radius, Ray r) {
    Vec3 oc = center - r.origin;
    double a = dot(r.direction, r.direction);
    double b = -2.0 * dot(r.direction, oc);
    double c = dot(oc, oc) - radius * radius;
    double discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return -1.0;
    } else {
        return (-b - sqrt(discriminant)) / (2.0 * a);
    }
}

Color ray_color(Ray r) {

    double t = hit_sphere(Point3(0, 0, -1), 0.5, r);
    if (t > 0.0) {
        Vec3 N = unit_vector(r.at(t) - Vec3(0, 0, -1));
        return 0.5 * Color(N.x + 1, N.y + 1, N.z + 1);
    }

    Vec3 unit_direction = unit_vector(r.direction);
    double a = 0.5 * (unit_direction.y + 1.0);
    return (1.0 - a) * Color(1.0, 1.0, 1.0) + a * Color(0.5, 0.7, 1.0);
}

struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension {
    int Width;
    int Height;
};

global_variable bool Running = true;
global_variable win32_offscreen_buffer GlobalBackBuffer;

win32_window_dimension Win32GetWindowDimension(HWND Window) {
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return Result;
}

internal void RenderRaytracer(win32_offscreen_buffer *Buffer) {
    int Pitch = Buffer->Pitch;
    uint8_t *Row = (uint8_t *)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y) {
        uint32_t *Pixel = (uint32_t *)Row;
        for (int X = 0; X < Buffer->Width; ++X) {

            uint8_t Blue = X;
            uint8_t Green = Y;

            *Pixel++ = ((Green << 8) | Blue);
        }
        Row += Buffer->Pitch;
    }
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width,
                                    int Height) {

    if (Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;
    Buffer->Pitch = Width * Buffer->BytesPerPixel;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = Width * Height * Buffer->BytesPerPixel;
    Buffer->Memory =
        VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32DisplayBufferToWindow(HDC DeviceContext, int WindowWidth,
                                         int WindowHeight,
                                         win32_offscreen_buffer *Buffer, int X,
                                         int Y, int Width, int Height) {

    // TODO: Aspect ratio correction

    StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0,
                  Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT MainWindowCallback(HWND WindowHandle, UINT Message, WPARAM wParam,
                           LPARAM lParam) {
    LRESULT Result = 0;

    switch (Message) {
    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_UP: {
            
        } break;
        }
    } break;

    case WM_SIZE: {
    } break;
    case WM_CLOSE:
    case WM_DESTROY: {
        Running = false;
    } break;
    case WM_PAINT: {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(WindowHandle, &Paint);
        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;

        win32_window_dimension WindowDimension =
            Win32GetWindowDimension(WindowHandle);
        Win32DisplayBufferToWindow(DeviceContext, WindowDimension.Width,
                                   WindowDimension.Height, &GlobalBackBuffer, X,
                                   Y, Width, Height);
        EndPaint(WindowHandle, &Paint);
    } break;
    default: {
        Result = DefWindowProc(WindowHandle, Message, wParam, lParam);
    } break;
    }

    return Result;
}

int APIENTRY WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance,
                     PSTR CommandLine, int ShowCode) {
    Win32ResizeDIBSection(&GlobalBackBuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
    WNDCLASSA wc = {};
    wc.lpfnWndProc = MainWindowCallback;
    wc.hInstance = Instance;
    wc.lpszClassName = (LPCSTR) "HandmadeHeroWindowClass";

    double aspect_ratio = 16.0 / 9.0;

    double viewport_height = 2.0;
    double viewport_width =
        viewport_height * (double(SCREEN_WIDTH) / SCREEN_HEIGHT);

    double focal_length = 1.0;
    Point3 CameraCenter = Point3(0, 0, 0);
    Vec3 viewport_u = Vec3(viewport_width, 0, 0);
    Vec3 viewport_v = Vec3(0, -viewport_height, 0);

    Vec3 pixel_delta_u = viewport_u / SCREEN_WIDTH;
    Vec3 pixel_delta_v = viewport_v / SCREEN_HEIGHT;

    Point3 viewport_upper_left = CameraCenter - Vec3(0, 0, focal_length) -
                                 viewport_u / 2 - viewport_v / 2;

    Point3 pixel00_loc =
        viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    if (RegisterClassA(&wc)) {

        HWND Window = CreateWindowA(wc.lpszClassName, (LPCSTR) "Handmade Hero",
                                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, 0, 0, Instance, 0);

        if (Window) {
            HDC DeviceContext = GetDC(Window);

            // SetWindowPos(Window, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

            // Render Raytracer

            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);

            {
                int Pitch = GlobalBackBuffer.Pitch;
                uint8_t *Row = (uint8_t *)GlobalBackBuffer.Memory;
                for (int Y = 0; Y < GlobalBackBuffer.Height; ++Y) {
                    uint32_t *Pixel = (uint32_t *)Row;
                    for (int X = 0; X < GlobalBackBuffer.Width; ++X) {

                        Point3 pixel_center = pixel00_loc +
                                              (X * pixel_delta_u) +
                                              (Y * pixel_delta_v);
                        Vec3 ray_direction = pixel_center - CameraCenter;
                        Ray r = Ray(CameraCenter, ray_direction);

                        Color pixel_color = ray_color(r);

                        *Pixel++ = pixel_color.toPixel();
                    }
                    Row += GlobalBackBuffer.Pitch;
                }
            }

            while (Running) {
                MSG Message;
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT) {
                        Running = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                win32_window_dimension dim = Win32GetWindowDimension(Window);
                Win32DisplayBufferToWindow(DeviceContext, dim.Width, dim.Height,
                                           &GlobalBackBuffer, 0, 0, dim.Width,
                                           dim.Height);
            }
        } else {
            // Failed to create window
        }
    } else {
        // Failed to register
    }
    return 0;
}
