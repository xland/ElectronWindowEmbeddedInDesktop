const { app, BrowserWindow, ipcMain } = require("electron");
const native = require("./build/Debug/native.node");
let win;

const createWindow = () => {
  win = new BrowserWindow({
    roundedCorners: false,
    width: 600,
    height: 800,
    enableLargerThanScreen: true,
    autoHideMenuBar: true,
    frame: false,
    transparent: true,
    show: true,
    webPreferences: {
      nodeIntegration: true,
      webSecurity: false,
      allowRunningInsecureContent: true,
      contextIsolation: false,
      webviewTag: true,
      spellcheck: false,
      disableHtmlFullscreenWindowResize: true,
    },
  });
  win.loadFile("index.html");
  win.webContents.openDevTools({
    mode: "detach",
  });
};
ipcMain.handle("show", () => {
  win.show();
  win.focus();
});
ipcMain.handle("close", () => {
  native.unEmbed(win.getNativeWindowHandle());
  win.close();
});
ipcMain.handle("detach", () => {
  let oldWin = win;
  oldWin.hide();
  native.unEmbed(oldWin.getNativeWindowHandle());
  createWindow();
  win.show();
  win.restore();
  win.setAlwaysOnTop(true);
  oldWin.close();
});
ipcMain.handle("attach", () => {
  native.embed(win.getNativeWindowHandle());
});
app.whenReady().then(() => {
  createWindow();
});
