# Quick Reference Card - Automotive Navigation

## 🚀 Common Commands

### Building
```powershell
# Full build (first time or after major changes)
.\scripts\build.bat

# Quick rebuild and run (development)
.\scripts\rebuild_and_run.bat

# Clean build (delete build folder first)
Remove-Item -Recurse -Force ..\build_Automotive
.\scripts\build.bat
```

### Running
```powershell
# Run application
.\scripts\run_integrated_nav.bat

# Run directly
..\install_Automotive\bin\nav_hmi_gui.exe
```

### Deployment
```powershell
# Build and deploy
.\scripts\build_and_deploy.bat

# Deploy only
.\scripts\deploy.bat

# Quick deploy (no full copy)
.\scripts\quick_deploy.bat

# Create package
.\scripts\create_package.bat
```

### Development
```powershell
# Build
.\scripts\build.bat

# Edit code...

# Test changes
.\scripts\rebuild_and_run.bat

# Repeat...
```

## 📁 Important Locations

| What | Where |
|------|-------|
| Source Code | `.\hmi\`, `.\common\` |
| Build Scripts | `.\scripts\` |
| Documentation | `.\docs\` |
| Configuration | `.\config\` |
| Build Output | `..\build_Automotive\` |
| Installed App | `..\install_Automotive\` |
| Executable | `..\install_Automotive\bin\nav_hmi_gui.exe` |
| Logs | `..\install_Automotive\logs\` |

## 📚 Documentation Quick Links

| Document | Purpose |
|----------|---------|
| [README.md](../README.md) | Project overview |
| [scripts/README.md](../scripts/README.md) | Scripts guide |
| [docs/BUILD_GUIDE.md](BUILD_GUIDE.md) | Detailed build |
| [docs/PROJECT_ORGANIZATION.md](PROJECT_ORGANIZATION.md) | Project structure |
| [docs/SERVICE_INTEGRATION.md](SERVICE_INTEGRATION.md) | Services architecture |

## 🔧 Troubleshooting

### Build fails - Qt not found
```powershell
# Set Qt path
$env:QT_DIR = "C:\Qt\6.6.1\msvc2019_64"
.\scripts\build.bat
```

### Build fails - CMake error
```powershell
# Clean and rebuild
Remove-Item -Recurse -Force ..\build_Automotive
.\scripts\build.bat
```

### App won't run - Missing DLL
```powershell
# Rebuild with proper Qt path
$env:QT_DIR = "C:\Qt\6.6.1\msvc2019_64"
.\scripts\build.bat
```

### Can't find executable
```powershell
# Check install directory
Get-ChildItem ..\install_Automotive\bin\
```

## 🎯 Quick Tasks

### Add new source file
1. Create file in `hmi/services/`, `hmi/ui/`, etc.
2. Add to appropriate `CMakeLists.txt`
3. Rebuild: `.\scripts\rebuild_and_run.bat`

### Update UI
1. Edit files in `hmi/ui/src/` or `hmi/ui/include/`
2. Rebuild: `.\scripts\rebuild_and_run.bat`

### Add new service
1. Create in `hmi/services/your_service/`
2. Add to `hmi/CMakeLists.txt`
3. Update service integration
4. Rebuild: `.\scripts\build.bat`

### Test changes
1. Make changes
2. Run: `.\scripts\rebuild_and_run.bat`
3. Check logs in `..\install_Automotive\logs\`

## 💡 Pro Tips

- **Fast iteration**: Use `rebuild_and_run.bat` during development
- **Clean build**: Delete `..\build_Automotive` before full rebuild
- **Check logs**: Look in `..\install_Automotive\logs\` for runtime errors
- **Git status**: `git status` to see what changed
- **Documentation**: All guides in `docs/` folder
- **Scripts help**: Read `scripts/README.md` for detailed script usage

## 🐛 Debug Commands

```powershell
# Check Qt installation
Get-ChildItem "C:\Qt\6.6.1\msvc2019_64"

# Check build output
Get-ChildItem ..\build_Automotive

# Check installed files
Get-ChildItem ..\install_Automotive -Recurse

# View recent logs
Get-Content ..\install_Automotive\logs\latest.log -Tail 50

# Git status
git status
git diff

# CMake reconfigure
cd ..\build_Automotive
cmake ..
```

## 📦 Git Workflow

```powershell
# Check status
git status

# Stage changes
git add .

# Commit
git commit -m "Your message"

# Push
git push origin test_branch

# Pull latest
git pull origin test_branch
```

## 🔄 Update Build Structure

If you pulled changes that modified build scripts:

```powershell
# Navigate to repo root
cd "d:\Data\My job\C++\Automotive"

# Run updated build script
.\scripts\build.bat

# Or rebuild
.\scripts\rebuild_and_run.bat
```

---

**Keep this file bookmarked for quick reference!**
