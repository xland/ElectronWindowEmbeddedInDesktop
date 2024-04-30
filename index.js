const { app, BrowserWindow, ipcMain } = require("electron");
const native = require("./build/Debug/native.node");
let win;

const createWindow = () => {
  win = new BrowserWindow({
    roundedCorners: false,
    width: 675,
    height: 680,
    enableLargerThanScreen: true,
    autoHideMenuBar: true,
    show: false,
    frame: false,
    transparent: true,
    skipTaskbar: true,
    focusable: false,
    type: "desktop",
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
  console.log("allen");
  native.embed(win.getNativeWindowHandle());
  win.show();
});
ipcMain.handle("close", () => {
  native.unEmbed(win.getNativeWindowHandle());
  win.close();
});
ipcMain.handle("detach", () => {
  win.hide();
  setTimeout(() => {
    native.unEmbed(win.getNativeWindowHandle());
  }, 600);
  setTimeout(() => {
    win.show();
  }, 1200);
});
ipcMain.handle("attach", () => {
  native.embed(win.getNativeWindowHandle());
});
app.whenReady().then(() => {
  createWindow();
});
