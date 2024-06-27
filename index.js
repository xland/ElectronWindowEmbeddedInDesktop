const { app, BrowserWindow, ipcMain } = require("electron");
const native = require("./build/Debug/native.node");

let arr = [];

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
  arr.push(win);
  return win;
  // win.webContents.openDevTools({
  //   mode: "detach",
  // });
};
ipcMain.handle("show", () => {
  win.show();
  win.focus();
});
ipcMain.handle("close", (e) => {
  let winTarget = BrowserWindow.fromWebContents(e.sender);
  native.unEmbed(winTarget.getNativeWindowHandle());
  winTarget.close();
});

ipcMain.handle("open", () => {
  createWindow();
});

ipcMain.handle("focus", (e) => {
  let winTarget = BrowserWindow.fromWebContents(e.sender);
  winTarget.focus();
  winTarget.setAlwaysOnTop(true);
});

ipcMain.handle("detach", (e) => {
  let winTarget = BrowserWindow.fromWebContents(e.sender);
  native.unEmbed(winTarget.getNativeWindowHandle());
  let index = arr.findIndex((v) => v === winTarget);
  arr.splice(index, 1);
  let win = createWindow();
  win.show();
  win.restore();
  win.setAlwaysOnTop(true);
  winTarget.close();
});
ipcMain.handle("attach", (e) => {
  let winTarget = BrowserWindow.fromWebContents(e.sender);
  native.embed(winTarget.getNativeWindowHandle());
});
app.whenReady().then(() => {
  createWindow();
});
