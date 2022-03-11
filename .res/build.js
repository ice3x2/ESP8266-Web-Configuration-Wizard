const fs = require('fs');
const Path = require('path');
const UglifyJS = require('uglify-js');
const HTMLMinify = require('html-minifier').minify;

let defPath = __dirname;
let defJsPath =  Path.join(defPath, 'js');
let defCssPath =  Path.join(defPath, 'css');
let buildPath = Path.join(defPath, 'build');
let buildJsPath = Path.join(defPath, 'build', 'js');
let buildCssPath = Path.join(defPath, 'build', 'css');



if (!fs.existsSync(buildPath)){
    fs.mkdirSync(buildPath);
}
if (!fs.existsSync(buildCssPath)){
    fs.mkdirSync(buildCssPath);
}
if (!fs.existsSync(buildJsPath)){
    fs.mkdirSync(buildJsPath);
}

function compressJs(fileName) {

    let originFile = Path.join(defJsPath, fileName);
    let destFile = Path.join(buildJsPath, fileName);
    let originScript = fs.readFileSync(originFile, {encoding: 'utf-8'});
    let uglyScript = UglifyJS.minify(originScript, {
        mangle: {
            toplevel: true,
            reserved: ['WifiConfig','TimeConfig', 'OptionConfig', 'MqttConfig', 'FinishView']
        },
        compress: true
    });
    fs.writeFileSync(destFile, uglyScript.code,{encoding: 'utf-8'} );
}

function compressHTML(fileName) {

    let originFile = Path.join(defPath, fileName);
    let destFile = Path.join(buildPath, fileName);

    let originHtml = fs.readFileSync(originFile, {encoding: 'utf-8'});
    let resultHtml = HTMLMinify(originHtml, {
        quoteCharacter: false,
        removeComments: true,
        minifyCSS: true,
        collapseWhitespace: true
    });
    resultHtml = resultHtml.replace(/\"/ig, "'");
    fs.writeFileSync(destFile, resultHtml,{encoding: 'utf-8'} );
}

function compressCSS(fileName) {

    let originFile = Path.join(defCssPath, fileName);
    let destFile = Path.join(buildCssPath, fileName);

    let originHtml = fs.readFileSync(originFile, {encoding: 'utf-8'});
    let resultHtml = HTMLMinify(originHtml, {
        quoteCharacter: false,
        removeComments: true,
        minifyCSS: true,
        collapseWhitespace: true
    });
    resultHtml = resultHtml.replace(/\"/ig, "'");
    fs.writeFileSync(destFile, resultHtml,{encoding: 'utf-8'} );
}

function makeResourceHpp() {
    let ajaxJs = fs.readFileSync(Path.join(buildJsPath, 'ajax.js'), 'utf-8').replace(/\"/ig, '\\\"').replace(/[.]html/ig, '');
    let appJs = fs.readFileSync(Path.join(buildJsPath, 'app.js'), 'utf-8').replace(/\"/ig, '\\\"').replace(/[.]html/ig, '');
    let envJs = "let DEV_URL = ''";
    let mainCss = fs.readFileSync(Path.join(buildCssPath, 'main.css'), 'utf-8').replace(/\"/ig, '\\\"').replace(/[.]html/ig, '');
    let finishHtml = fs.readFileSync(Path.join(buildPath, 'finish.html'), 'utf-8').replace(/\"/ig, '\\\"').replace(/[.]html/ig, '');
    let mqttHtml = fs.readFileSync(Path.join(buildPath, 'mqtt.html'), 'utf-8').replace(/\"/ig, '\\\"').replace(/[.]html/ig, '');
    let optionHtml = fs.readFileSync(Path.join(buildPath, 'option.html'), 'utf-8').replace(/\"/ig, '\\\"').replace(/[.]html/ig, '');
    let wifiHtml = fs.readFileSync(Path.join(buildPath, 'wifi.html'), 'utf-8').replace(/\"/ig, '\\\"').replace(/[.]html/ig, '');
    let timeHtml = fs.readFileSync(Path.join(buildPath, 'time.html'), 'utf-8').replace(/\"/ig, '\\\"').replace(/[.]html/ig, '');

    let src = "";
    src +=  "#define RES_WIFI_HTML \"" + wifiHtml + "\"\n";
    src +=  "#define RES_TIME_HTML \"" + timeHtml + "\"\n";
    src +=  "#define RES_MQTT_HTML \"" + mqttHtml + "\"\n";
    src +=  "#define RES_OPTION_HTML \"" + optionHtml + "\"\n";
    src +=  "#define RES_FINISH_HTML \"" + finishHtml + "\"\n";
    src +=  "#define RES_MAIN_CSS \"" + mainCss + "\"\n";
    src +=  "#define RES_ENV_JS \"" + envJs + "\"\n";
    src +=  "#define RES_APP_JS \"" + appJs + "\"\n";
    src +=  "#define RES_AJAX_JS \"" + ajaxJs + "\"\n";


    fs.writeFileSync(Path.join(buildPath,'Resources.hpp'), src, 'utf-8');

}


fs.copyFileSync(Path.join(defJsPath, 'ajax.js'),Path.join(buildJsPath, 'ajax.js'));
fs.copyFileSync(Path.join(defJsPath, 'env.js'),Path.join(buildJsPath, 'env.js'));


compressHTML('wifi.html');
compressHTML('finish.html');
compressHTML('mqtt.html');
compressHTML('option.html');
compressHTML('time.html');
compressJs('app.js');
compressCSS('main.css');

makeResourceHpp();


fs.copyFileSync(Path.join(buildPath, 'Resources.hpp'),Path.join(defPath,'..', 'Resources.hpp'));