#include <cmath>
#include <cstdint>
#include <iostream>
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
    double length_squared() { return x * x + y * y + z * z; }
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

struct HitRecord {
    Point3 p;
    Vec3 normal;
    double t;
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
    double a = r.direction.length_squared();
    double h = dot(r.direction, oc);
    double c = oc.length_squared() - radius * radius;
    double discriminant = h * h - a * c;

    if (discriminant < 0) {
        return -1.0;
    } else {
        return (h - sqrt(discriminant)) / a;
    }
}

Color ray_color(Ray r) {
    Vec3 light = Vec3(-1, -1, -1);
    double t = hit_sphere(Point3(0, 0, -1), 0.5, r);
    if (t > 0.0) {
        Vec3 N = unit_vector(r.at(t) - Vec3(0, 0, -1));

        double d = fmax(dot(N, -1 * light), 0.0f);

        return d * 0.5 * Color(0.7, 0.4, 0.3);
    }

    // Vec3 unit_direction = unit_vector(r.direction);
    // double a = 0.5 * (unit_direction.y + 1.0);

    return Color(0, 0, 0);
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

    double aspect_ratio = (double(SCREEN_WIDTH) / SCREEN_HEIGHT);

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

        RECT Rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        DWORD Style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        BOOL Menu = false;

        AdjustWindowRect(&Rect, Style, Menu);

        HWND Window = CreateWindowA(
            wc.lpszClassName, (LPCSTR) "Handmade Raycaster", Style, 100, 100,
            Rect.right - Rect.left, Rect.bottom - Rect.top, 0, 0, Instance, 0);

        if (Window) {
            HDC DeviceContext = GetDC(Window);

            // Create and populate world


            // Render Raytracer

            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);

            LARGE_INTEGER start;
            QueryPerformanceCounter(&start);

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
            LARGE_INTEGER end;
            QueryPerformanceCounter(&end);

            std::clog << "Raytracing took: "
                      << (double)(end.QuadPart - start.QuadPart) / freq.QuadPart
                      << " seconds \n";
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
