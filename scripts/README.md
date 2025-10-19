# Build Scripts Guide

Thư mục này chứa tất cả các scripts để build, test và deploy dự án.

## 📁 Cấu trúc Scripts

```
scripts/
├── build.bat                    # Build script chính (Windows)
├── build.sh                     # Build script chính (Linux/macOS)
├── rebuild_and_run.bat          # Rebuild toàn bộ và chạy ứng dụng
├── run_integrated_nav.bat       # Chạy ứng dụng đã build
├── build_and_deploy.bat         # Build và deploy cùng lúc
├── create_package.bat           # Tạo package để distribute
├── deploy.bat                   # Deploy ứng dụng
├── quick_deploy.bat             # Quick deploy (skip build)
├── deploy.config                # Cấu hình deployment
└── README.md                    # File này
```

---

## 🚀 Cách Sử Dụng

### 1. Build Cơ Bản

**Windows:**
```powershell
cd "d:\Data\My job\C++\Automotive"
.\scripts\build.bat
```

**Linux/macOS:**
```bash
cd /path/to/Automotive
./scripts/build.sh
```

**Kết quả:**
- Build output → `../Automotive-build/` (ngoài repo)
- Install files → `../Automotive-install/` (ngoài repo)

---

### 2. Rebuild và Chạy

Rebuild toàn bộ project và tự động chạy ứng dụng:

```powershell
.\scripts\rebuild_and_run.bat
```

**Các bước thực hiện:**
1. Xóa build directory cũ
2. Configure lại CMake
3. Build project
4. Install files
5. Chạy `nav_hmi_gui.exe`

---

### 3. Chỉ Chạy Ứng Dụng

Nếu đã build xong, chỉ cần chạy:

```powershell
.\scripts\run_integrated_nav.bat
```

**Yêu cầu:**
- Đã build và install thành công
- File `nav_hmi_gui.exe` tồn tại trong install directory

---

### 4. Build và Deploy

Build và deploy ứng dụng đến target location:

```powershell
.\scripts\build_and_deploy.bat
```

**Các bước:**
1. Build project (Release mode)
2. Install files
3. Copy đến deployment location (theo `deploy.config`)
4. Tạo shortcuts (nếu cần)

---

### 5. Tạo Package

Tạo package để phân phối:

```powershell
.\scripts\create_package.bat
```

**Output:**
- ZIP file chứa executable + dependencies
- Tên file: `AutomotiveNav_v{VERSION}_{DATE}.zip`
- Location: `../Automotive-packages/`

---

### 6. Quick Deploy

Deploy nhanh mà không build lại:

```powershell
.\scripts\quick_deploy.bat
```

**Sử dụng khi:**
- Đã build xong, chỉ cần copy files mới
- Test deployment trên nhiều máy
- Update configuration files

---

## ⚙️ Cấu Hình

### Build Configuration

Edit `build.bat` hoặc `build.sh`:

```bat
REM Thay đổi build type
set BUILD_TYPE=Release          REM hoặc Debug, RelWithDebInfo

REM Thay đổi Qt path
set Qt6_DIR=C:\Qt\6.6.1\msvc2019_64

REM Thay đổi compiler
set CMAKE_GENERATOR=Visual Studio 17 2022
```

### Deployment Configuration

Edit `deploy.config`:

```ini
DEPLOY_PATH=C:\DeployedApps\Navigation
CREATE_SHORTCUTS=true
COPY_CONFIG=true
BACKUP_OLD=true
```

---

## 🔧 Troubleshooting

### Lỗi: "Qt6_DIR not found"

**Giải pháp:**
```powershell
# Set Qt path trong environment
$env:Qt6_DIR = "C:\Qt\6.6.1\msvc2019_64"
.\scripts\build.bat
```

### Lỗi: "CMake not found"

**Giải pháp:**
- Install CMake: https://cmake.org/download/
- Thêm CMake vào PATH
- Restart terminal

### Lỗi: "Build directory already exists"

**Giải pháp:**
```powershell
# Xóa build directory cũ
Remove-Item -Recurse -Force "..\Automotive-build"
.\scripts\build.bat
```

### Lỗi: "nav_hmi_gui.exe not found"

**Giải pháp:**
```powershell
# Build lại project
.\scripts\rebuild_and_run.bat
```

---

## 📊 Build Output Locations

```
Automotive/                          # Repo (source code)
    ├── CMakeLists.txt
    ├── scripts/                     # Build scripts (đây)
    └── ...

Automotive-build/                    # Build artifacts (ngoài repo)
    ├── CMakeCache.txt
    ├── compile_commands.json
    └── ...

Automotive-install/                  # Installed files (ngoài repo)
    ├── bin/
    │   ├── nav_hmi_gui.exe
    │   ├── Qt6Core.dll
    │   └── ...
    └── etc/
        └── navigation.conf

Automotive-packages/                 # Packaged releases (ngoài repo)
    └── AutomotiveNav_v1.0_20251019.zip
```

**Lợi ích:**
- Source code sạch, không có build artifacts
- Dễ dàng clean/rebuild
- Không commit nhầm build files
- Dễ backup code riêng, build riêng

---

## 🎯 Workflow Thông Thường

### Developer Workflow

```powershell
# 1. Pull code mới
git pull origin test_branch

# 2. Rebuild
.\scripts\rebuild_and_run.bat

# 3. Code changes...

# 4. Test nhanh
.\scripts\run_integrated_nav.bat

# 5. Build lại sau khi sửa
.\scripts\build.bat
.\scripts\run_integrated_nav.bat
```

### Release Workflow

```powershell
# 1. Update version trong CMakeLists.txt
# 2. Build Release
.\scripts\build.bat

# 3. Test thoroughly
.\scripts\run_integrated_nav.bat

# 4. Create package
.\scripts\create_package.bat

# 5. Deploy
.\scripts\deploy.bat

# 6. Tag release
git tag v1.0.0
git push origin v1.0.0
```

---

## 📝 Notes

- **Luôn build ở root directory** của repo, không phải trong `scripts/`
- **Scripts tự động detect** Qt, CMake, compiler
- **Build directory** được tạo ngoài repo để giữ code sạch
- **Install directory** chứa files ready-to-run

---

**Last Updated:** October 19, 2025  
**Maintainer:** Navigation Team
