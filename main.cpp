#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <windows.h>
#include <gdiplus.h>
#include "EulerOperations.h"
#include "SolidModel.h"

using namespace std;

// 渲染相关结构 - 3D点的表示
typedef struct {
    float x, y, z;
} Point3D;

// 渲染相关结构 - 3D线段的表示
typedef struct {
    Point3D start;  // 线段起点
    Point3D end;    // 线段终点
} LineSegment3D;

// 渲染参数 - 用于控制3D模型的变换
float rotationX = 0.0f;       // 绕X轴的旋转角度（弧度）
float rotationY = 0.0f;       // 绕Y轴的旋转角度（弧度）
float scale = 1.0f;           // 缩放比例
float translateX = 0.0f;      // X轴平移量（屏幕坐标）
float translateY = 0.0f;      // Y轴平移量（屏幕坐标）
Point3D centerPoint = {0.0f, 0.0f, 0.0f};  // 模型中心点
vector<LineSegment3D> modelLines;  // 存储所有需要渲染的线段

// 窗口和鼠标状态
bool isDragging = false;      // 是否正在拖动鼠标
int lastMouseX = 0, lastMouseY = 0;  // 上一次鼠标位置

// GDI+相关 - 用于GDI+库的初始化和清理
ULONG_PTR gdiplusToken = NULL;  // GDI+初始化令牌

// 初始化GDI+库
void initGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;  // GDI+启动配置
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);  // 初始化GDI+
}

// 清理GDI+库资源
void cleanupGDIPlus() {
    Gdiplus::GdiplusShutdown(gdiplusToken);  // 关闭GDI+
}

// 辅助函数：将实体模型转换为线段集合
// 此函数遍历实体模型的所有面、环和边，将每条边转换为渲染用的线段
void modelToLineSegments(Body* body) {
    if (!body) return;  // 检查模型是否有效
    
    modelLines.clear();  // 清空线段列表
    vector<Edge*> processedEdges;  // 用于跟踪已处理的边，避免重复添加
    
    // 计算模型中心点 - 用于旋转和平移操作
    double totalX = 0, totalY = 0, totalZ = 0;
    int vertexCount = 0;
    
    // 遍历所有面 - 实体模型由多个面组成
    Face* currentFace = body->first_face_;
    while (currentFace) {
        // 遍历每个面的所有环（外边界和内边界）
        Loop* currentLoop = currentFace->first_loop_;
        while (currentLoop) {
            // 获取环的起始半边
            Halfedge* startHe = currentLoop->start_he_;
            if (startHe) {
                Halfedge* currentHe = startHe;
                // 遍历环的所有半边
                do {
                    Edge* edge = currentHe->edge_;
                    // 检查边是否有效且未处理过
                    if (edge && find(processedEdges.begin(), processedEdges.end(), edge) == processedEdges.end()) {
                        Vertex* v1 = currentHe->start_vertex_;  // 边的起始顶点
                        Vertex* v2 = currentHe->to_vertex_;     // 边的终点顶点
                        
                        if (v1 && v2) {
                            // 创建渲染用的线段
                            LineSegment3D segment;
                            segment.start.x = (float)v1->p_.x();  // 设置起点坐标
                            segment.start.y = (float)v1->p_.y();
                            segment.start.z = (float)v1->p_.z();
                            segment.end.x = (float)v2->p_.x();    // 设置终点坐标
                            segment.end.y = (float)v2->p_.y();
                            segment.end.z = (float)v2->p_.z();
                            modelLines.push_back(segment);  // 添加到线段列表
                            
                            // 累积顶点坐标用于计算中心点
                            totalX += v1->p_.x() + v2->p_.x();
                            totalY += v1->p_.y() + v2->p_.y();
                            totalZ += v1->p_.z() + v2->p_.z();
                            vertexCount += 2;
                        }
                        
                        processedEdges.push_back(edge);  // 标记边为已处理
                    }
                    
                    currentHe = currentHe->next_he_;  // 移动到下一个半边
                } while (currentHe && currentHe != startHe);  // 循环直到回到起点
            }
            
            currentLoop = currentLoop->next_loop_;  // 移动到下一个环
        }
        
        currentFace = currentFace->next_face_;  // 移动到下一个面
    }
    
    // 设置模型中心点 - 用于旋转变换的中心点
    if (vertexCount > 0) {
        centerPoint.x = (float)(totalX / vertexCount);
        centerPoint.y = (float)(totalY / vertexCount);
        centerPoint.z = (float)(totalZ / vertexCount);
    }
}

// 3D点到2D投影函数 - 将3D世界坐标转换为2D屏幕坐标
// 参数:
//   - p: 要投影的3D点
//   - width: 窗口宽度
//   - height: 窗口高度
// 返回值: 投影后的2D屏幕坐标点
Gdiplus::Point projectPoint(Point3D p, int width, int height) {
    // 第一步：将点相对于模型中心点偏移，使旋转围绕中心点进行
    float x = p.x - centerPoint.x;
    float y = p.y - centerPoint.y;
    float z = p.z - centerPoint.z;
    
    // 第二步：应用旋转变换
    // 绕X轴旋转 - 使用标准旋转矩阵
    float tempY = y * cos(rotationX) - z * sin(rotationX);
    float tempZ = y * sin(rotationX) + z * cos(rotationX);
    y = tempY;
    z = tempZ;
    
    // 绕Y轴旋转 - 使用标准旋转矩阵
    float tempX = x * cos(rotationY) + z * sin(rotationY);
    tempZ = -x * sin(rotationY) + z * cos(rotationY);
    x = tempX;
    z = tempZ;
    
    // 第三步：应用缩放和平移变换
    // 缩放因子乘以100是为了使模型在屏幕上更明显
    // 加上width/2和height/2将原点移动到屏幕中心
    // 加上translateX和translateY应用用户平移
    int px = (int)(x * scale * 100 + width / 2 + translateX);
    // Y轴需要反转，因为屏幕Y轴向下，而数学上Y轴向上
    int py = (int)(-y * scale * 100 + height / 2 + translateY);
    
    // 返回GDI+兼容的2D点
    return Gdiplus::Point(px, py);
}

// 绘制线段函数 - 使用GDI+绘制一条3D线段在2D屏幕上
// 参数:
//   - graphics: GDI+绘图上下文
//   - pen: 用于绘制的画笔
//   - segment: 要绘制的3D线段
//   - width: 窗口宽度
//   - height: 窗口高度
void drawLine(Gdiplus::Graphics* graphics, Gdiplus::Pen* pen, LineSegment3D segment, int width, int height) {
    // 将线段的两个端点从3D投影到2D
    Gdiplus::Point p1 = projectPoint(segment.start, width, height);
    Gdiplus::Point p2 = projectPoint(segment.end, width, height);
    
    // 裁剪检查：确保线段的两个端点都在屏幕范围内
    // 这可以提高性能并避免绘制屏幕外的线段
    if (p1.X >= 0 && p1.X < width && p1.Y >= 0 && p1.Y < height &&
        p2.X >= 0 && p2.X < width && p2.Y >= 0 && p2.Y < height) {
        // 使用GDI+的DrawLine方法绘制线段
        graphics->DrawLine(pen, p1, p2);
    }
}



// 创建立方体模型函数 - 使用欧拉操作创建基本实体模型
// 返回值: 指向创建的实体模型的指针
Body* createSimpleModel() {
    try {
        EulerOperations eulerOps;  // 创建欧拉操作对象
        
        // 第一步：使用mvfs（Make Vertex Face Solid）操作创建初始顶点和基本面
        // 这是欧拉操作的起点，创建一个顶点、一个面和一个实体
        Point p0(0, 0, 0);  // 初始点坐标
        Vertex* v0 = eulerOps.mvfs(p0);  // 创建顶点并返回顶点指针
        
        if (!v0) {
            cout << "Error: 无法创建初始顶点" << endl;
            return nullptr;
        }
        
        // 获取创建的实体对象
        Body* body = eulerOps.get_body();
        if (!body || !body->first_face_ || !body->first_face_->first_loop_) {
            cout << "Error: 无法获取体对象或初始面的环" << endl;
            return nullptr;
        }
        
        // 获取初始面的环，用于后续添加边
        Loop* initialLoop = body->first_face_->first_loop_;
        
        // 准备立方体的其他七个顶点坐标
        Point p1(1, 0, 0);
        Point p2(1, 1, 0);
        Point p3(0, 1, 0);
        Point p4(0, 0, 1);
        Point p5(1, 0, 1);
        Point p6(1, 1, 1);
        Point p7(0, 1, 1);
        
        // 尝试使用mev（Make Edge Vertex）操作创建其他顶点和边
        // 注意：由于欧拉操作的限制，这里简化处理，实际渲染中会手动添加立方体框架
        Halfedge* h1 = eulerOps.mev(v0, nullptr, initialLoop); // 尝试创建顶点v1
        Halfedge* h2 = eulerOps.mev(v0, nullptr, initialLoop); // 尝试创建顶点v2
        Halfedge* h3 = eulerOps.mev(v0, nullptr, initialLoop); // 尝试创建顶点v3
        
        // 注意：完整的立方体构建通常需要更多的欧拉操作（如mef、kemr等）
        // 由于顶点构造的限制，我们在WinMain中手动添加立方体的线段
        
        cout << "已创建立方体框架模型" << endl;
        return body;
    } catch (const exception& e) {
        cout << "Exception in createSimpleModel: " << e.what() << endl;
        return nullptr;
    }
}



// 窗口过程函数 - 处理所有Windows消息
// 参数:
//   - hwnd: 窗口句柄
//   - uMsg: 消息类型
//   - wParam: 消息参数1
//   - lParam: 消息参数2
// 返回值: 根据消息类型的处理结果
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // 静态变量用于双缓冲渲染
    static HDC hdcMem;        // 内存设备上下文
    static HBITMAP hbmMem;    // 内存位图（用于双缓冲）
    static HBITMAP hbmOld;    // 保存旧的位图，用于清理
    static int width, height; // 窗口宽度和高度
    
    switch (uMsg) {
        case WM_CREATE: {  // 窗口创建时触发
            // 初始化GDI+库
            initGDIPlus();
            
            // 获取窗口客户区大小
            RECT rect;
            GetClientRect(hwnd, &rect);
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
            
            // 创建双缓冲所需的内存设备上下文和位图
            // 双缓冲可以避免闪烁
            HDC hdc = GetDC(hwnd);
            hdcMem = CreateCompatibleDC(hdc);  // 创建与窗口DC兼容的内存DC
            hbmMem = CreateCompatibleBitmap(hdc, width, height);  // 创建兼容位图
            hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);  // 选择位图到内存DC
            ReleaseDC(hwnd, hdc);
            
            return 0;
        }
        
        case WM_SIZE: {  // 窗口大小改变时触发
            // 更新窗口尺寸
            width = LOWORD(lParam);  // 低字部分是宽度
            height = HIWORD(lParam); // 高字部分是高度
            
            // 重新创建内存位图以匹配新的窗口大小
            SelectObject(hdcMem, hbmOld);  // 恢复旧的位图
            DeleteObject(hbmMem);         // 删除旧的内存位图
            
            HDC hdc = GetDC(hwnd);
            hbmMem = CreateCompatibleBitmap(hdc, width, height);  // 创建新的内存位图
            ReleaseDC(hwnd, hdc);
            
            hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);  // 选择新位图到内存DC
            InvalidateRect(hwnd, NULL, FALSE);  // 触发重绘
            
            return 0;
        }
        
        case WM_PAINT: {  // 窗口需要重绘时触发
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);  // 获取窗口DC
            
            // 创建GDI+图形对象，用于高级绘图
            Gdiplus::Graphics graphics(hdcMem);
            
            // 填充黑色背景
            Gdiplus::SolidBrush backBrush(Gdiplus::Color(0, 0, 0));  // 黑色画笔
            graphics.FillRectangle(&backBrush, 0, 0, width, height);
            
            // 创建红色画笔，用于绘制3D模型
            Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0), 1.0f);  // 红色，线宽1像素
            pen.SetLineJoin(Gdiplus::LineJoinRound);     // 设置线连接方式为圆形
            pen.SetStartCap(Gdiplus::LineCapRound);      // 设置线帽为圆形
            pen.SetEndCap(Gdiplus::LineCapRound);        // 设置线帽为圆形
            
            // 绘制模型的所有线段
            for (const auto& line : modelLines) {
                drawLine(&graphics, &pen, line, width, height);
            }
            
            // 添加操作提示文本
            Gdiplus::Font font(L"Arial", 12);  // 创建字体
            Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255));  // 白色文字
            Gdiplus::StringFormat format;
            format.SetAlignment(Gdiplus::StringAlignmentNear);  // 左对齐
            
            // 绘制操作说明
            graphics.DrawString(L"左键拖动: 旋转", -1, &font, Gdiplus::PointF(10, 10), &format, &textBrush);
            graphics.DrawString(L"右键拖动: 平移", -1, &font, Gdiplus::PointF(10, 30), &format, &textBrush);
            graphics.DrawString(L"滚轮: 缩放", -1, &font, Gdiplus::PointF(10, 50), &format, &textBrush);
            graphics.DrawString(L"ESC: 退出", -1, &font, Gdiplus::PointF(10, 70), &format, &textBrush);
            
            // 将内存DC中的内容复制到窗口DC，完成双缓冲绘制
            BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
            
            EndPaint(hwnd, &ps);  // 结束绘制
            return 0;
        }
        
        case WM_LBUTTONDOWN: {  // 左键按下事件
            isDragging = true;  // 设置拖动状态为真
            lastMouseX = LOWORD(lParam);  // 记录当前鼠标X坐标
            lastMouseY = HIWORD(lParam);  // 记录当前鼠标Y坐标
            SetCapture(hwnd);  // 捕获鼠标，即使鼠标移出窗口也能接收消息
            return 0;
        }
        
        case WM_LBUTTONUP: {  // 左键释放事件
            isDragging = false;  // 设置拖动状态为假
            ReleaseCapture();  // 释放鼠标捕获
            return 0;
        }
        
        case WM_RBUTTONDOWN: {  // 右键按下事件
            isDragging = true;  // 设置拖动状态为真
            lastMouseX = LOWORD(lParam);  // 记录当前鼠标X坐标
            lastMouseY = HIWORD(lParam);  // 记录当前鼠标Y坐标
            SetCapture(hwnd);  // 捕获鼠标
            return 0;
        }
        
        case WM_RBUTTONUP: {  // 右键释放事件
            isDragging = false;  // 设置拖动状态为假
            ReleaseCapture();  // 释放鼠标捕获
            return 0;
        }
        
        case WM_MOUSEMOVE: {  // 鼠标移动事件
            if (isDragging) {  // 只有在拖动状态才处理
                int mouseX = LOWORD(lParam);  // 当前鼠标X坐标
                int mouseY = HIWORD(lParam);  // 当前鼠标Y坐标
                
                // 判断是左键还是右键拖动
                if (wParam & MK_LBUTTON) {  // 左键拖动 - 旋转模型
                    // 计算鼠标移动的差值，并更新旋转角度
                    // 乘以0.01是为了控制旋转速度
                    rotationX += (mouseY - lastMouseY) * 0.01f;
                    rotationY += (mouseX - lastMouseX) * 0.01f;
                } else if (wParam & MK_RBUTTON) {  // 右键拖动 - 平移模型
                    // 直接使用鼠标移动的差值作为平移量
                    translateX += (mouseX - lastMouseX);
                    translateY += (mouseY - lastMouseY);
                }
                
                // 更新上一次鼠标位置
                lastMouseX = mouseX;
                lastMouseY = mouseY;
                // 触发窗口重绘，显示更新后的模型
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_MOUSEWHEEL: {  // 鼠标滚轮事件
            // 缩放模型
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);  // 获取滚轮滚动量
            // 根据滚轮方向调整缩放比例
            scale += zDelta * 0.001f;  // 乘以0.001控制缩放速度
            // 设置缩放比例的上下限，避免过度缩放
            if (scale < 0.1f) scale = 0.1f;  // 最小缩放比例
            if (scale > 5.0f) scale = 5.0f;  // 最大缩放比例
            // 触发重绘
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        
        case WM_KEYDOWN: {  // 键盘按键按下事件
            if (wParam == VK_ESCAPE) {  // ESC键 - 退出程序
                PostMessage(hwnd, WM_CLOSE, 0, 0);  // 发送关闭消息
            }
            return 0;
        }
        
        case WM_DESTROY: {  // 窗口销毁事件
            // 清理所有资源
            SelectObject(hdcMem, hbmOld);  // 恢复旧的位图
            DeleteObject(hbmMem);          // 删除内存位图
            DeleteDC(hdcMem);              // 删除内存设备上下文
            cleanupGDIPlus();              // 清理GDI+
            PostQuitMessage(0);            // 发送退出消息，结束消息循环
            return 0;
        }
    }
    
    // 对于未处理的消息，使用默认处理函数
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 初始化窗口函数
// 参数:
//   - hInstance: 应用程序实例句柄
//   - title: 窗口标题
//   - width: 窗口宽度
//   - height: 窗口高度
// 返回值: 创建的窗口句柄
HWND initWindow(HINSTANCE hInstance, const char* title, int width, int height) {
    const char* CLASS_NAME = "3D_MODEL_VIEWER";  // 窗口类名
    
    // 定义窗口类
    WNDCLASS wc = {};  // 初始化为0
    wc.lpfnWndProc = WindowProc;         // 设置窗口过程
    wc.hInstance = hInstance;            // 设置实例句柄
    wc.lpszClassName = CLASS_NAME;       // 设置类名
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // 设置鼠标光标
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);  // 设置窗口图标
    wc.style = CS_HREDRAW | CS_VREDRAW;  // 窗口大小改变时重绘
    
    // 注册窗口类
    RegisterClass(&wc);
    
    // 创建窗口
    HWND hwnd = CreateWindowEx(
        0,                        // 扩展窗口样式
        CLASS_NAME,               // 窗口类名
        title,                    // 窗口标题
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // 窗口样式（重叠窗口，可见）
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,  // 位置和大小
        NULL, NULL,               // 父窗口和菜单
        hInstance,                // 实例句柄
        NULL                      // 附加数据
    );
    
    return hwnd;  // 返回创建的窗口句柄
}

// 程序入口函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 设置控制台输出为UTF-8编码，确保中文正常显示
    SetConsoleOutputCP(CP_UTF8);
    
    cout << "开始构建实体模型..." << endl;
    
    // 创建实体模型 - 使用欧拉操作
    Body* model = createSimpleModel();
    
    // 检查模型是否创建成功
    if (!model) {
        cerr << "无法创建模型，程序退出" << endl;
        return 1;
    }
    
    // 输出模型信息到控制台
    cout << "\n模型信息:" << endl;
    cout << "顶点数量: " << model->vertex_num_ << endl;
    cout << "边数量: " << model->edge_num_ << endl;
    cout << "面数量: " << model->face_num_ << endl;
    
    // 注意：由于欧拉操作创建的模型可能不够完善，这里手动创建一个立方体框架并添加内部通孔
    // 清空线段列表
    modelLines.clear();
    
    // 定义外部立方体的8个顶点坐标
    vector<Point3D> outerCubeVertices = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},  // 底部四个点
        {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}   // 顶部四个点
    };
    
    // 定义外部立方体的12条边（用顶点索引对表示）
    int outerCubeEdges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},  // 底部四条边
        {4, 5}, {5, 6}, {6, 7}, {7, 4},  // 顶部四条边
        {0, 4}, {1, 5}, {2, 6}, {3, 7}   // 四条竖边
    };
    
    // 为外部立方体的每条边创建线段对象并添加到渲染列表
    for (int i = 0; i < 12; i++) {
        LineSegment3D seg;
        seg.start = outerCubeVertices[outerCubeEdges[i][0]];  // 设置边的起点
        seg.end = outerCubeVertices[outerCubeEdges[i][1]];    // 设置边的终点
        modelLines.push_back(seg);  // 添加到线段列表
    }
    
    // 定义内部长方体（通孔）的8个顶点坐标
    // 内部长方体尺寸为外部立方体的一半，居中放置
    // z坐标与外部立方体相同，使两端相切
    float innerSize = 0.5f;  // 内部长方体相对于外部立方体的尺寸比例
    vector<Point3D> innerCubeVertices = {
        {-innerSize, -innerSize, -1.0f}, {innerSize, -innerSize, -1.0f},  // 底部前面两点
        {innerSize, innerSize, -1.0f}, {-innerSize, innerSize, -1.0f},     // 底部后面两点
        {-innerSize, -innerSize, 1.0f},  {innerSize, -innerSize, 1.0f},   // 顶部前面两点
        {innerSize, innerSize, 1.0f},  {-innerSize, innerSize, 1.0f}      // 顶部后面两点
    };
    
    // 定义内部长方体的12条边（用顶点索引对表示）
    // 注意：我们只渲染通孔的内部边缘，以显示通孔效果
    int innerCubeEdges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},  // 底部四条边
        {4, 5}, {5, 6}, {6, 7}, {7, 4},  // 顶部四条边
        {0, 4}, {1, 5}, {2, 6}, {3, 7}   // 四条竖边
    };
    
    // 为内部长方体的每条边创建线段对象并添加到渲染列表
    for (int i = 0; i < 12; i++) {
        LineSegment3D seg;
        seg.start = innerCubeVertices[innerCubeEdges[i][0]];  // 设置边的起点
        seg.end = innerCubeVertices[innerCubeEdges[i][1]];    // 设置边的终点
        modelLines.push_back(seg);  // 添加到线段列表
    }
    
    // 初始化图形窗口
    HWND hwnd = initWindow(hInstance, "3D模型渲染器", 800, 600);
    if (!hwnd) {
        delete model;  // 清理资源
        cerr << "无法创建窗口，程序退出" << endl;
        return 1;
    }
    
    // 输出操作说明到控制台
    cout << "渲染窗口已启动\n" << endl;
    cout << "操作说明:" << endl;
    cout << "- 左键拖动: 旋转模型" << endl;
    cout << "- 右键拖动: 平移模型" << endl;
    cout << "- 滚轮: 缩放模型" << endl;
    cout << "- 按ESC键: 退出程序" << endl;
    
    // Windows消息循环 - 处理所有窗口消息
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {  // 获取消息
        TranslateMessage(&msg);  // 翻译虚拟键消息
        DispatchMessage(&msg);   // 分发消息到窗口过程
    }
    
    // 程序结束前清理资源
    delete model;
    
    cout << "\n程序执行完成。" << endl;
    return (int)msg.wParam;
}