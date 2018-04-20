import {nodes} from "./emulator/NodeStore";
import {setMaxNodes, shutdown, setData} from "./emulator/Emulator";

const api = require('bluzelle');

export const reset = () =>
    require('./emulator/Emulator').reset(api.getUuid());


const undo = () =>
    browser.click('.glyphicon-chevron-left');

const redo = () =>
    browser.click('.glyphicon-chevron-right');

export const checkUndo = ({verifyDo, verifyUndo}) => {

    verifyDo();
    undo();
    verifyUndo();
    redo();
    verifyDo();


};