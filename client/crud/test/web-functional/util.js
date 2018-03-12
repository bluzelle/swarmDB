import {nodes} from "./emulator/NodeStore";
import {setMaxNodes, shutdown, setData} from "./emulator/Emulator";

export const reset = () => {
    wrapAsync(() => Promise.all(shutdown()))();
    setMaxNodes(1);
    setData({});
    browser.waitUntil(() => nodes.keys().length);
};



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