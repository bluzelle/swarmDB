import {isObservableArray} from "mobx";
import PropTypes from 'prop-types';

export const commandQueue = observable([]);
export const currentPosition = observable(0);

const revert = targetPosition => {
    const cp = currentPosition.get();
    
    if (cp > targetPosition) {
        const prev = commandQueue[cp-1];

        commandQueue[cp].undoIt(prev).then(() => {

            currentPosition.set(cp - 1);
            revert(targetPosition);

        });
    }

    if (cp < targetPosition) {
        
        commandQueue[cp + 1].doIt().then(() => {

            currentPosition.set(cp + 1);
            revert(targetPosition);

        });
    }
};

commandQueue.push({
    message: 'Initial state',
    revert: revert.bind(this, 0)
});


export const canUndo = () => currentPosition.get() > 0;

export const undo = () =>
    canUndo() && revert(currentPosition.get() - 1);


export const canRedo = () => currentPosition.get() < commandQueue.length - 1;

export const redo = () =>
    canRedo() && revert(currentPosition.get() + 1);


// Caution: the consecutive execution of undoIt and doIt should cancel
// each other out exactly. If the action is to, for instance, create a new
// object, the object should have the same identity as it had before
// undoing and redoing.

export const execute = ({ doIt, undoIt, message }) => 
    new Promise(resolve => {

    doIt().then(resolve);

    currentPosition.set(currentPosition.get() + 1);
    deleteFuture();

    commandQueue.push({
        revert: revert.bind(this, currentPosition.get()),
        doIt,
        undoIt,
        message
    });
});


const deleteFuture = () =>
    (currentPosition.get() >= 0) && (commandQueue.length = currentPosition.get());

export const removePreviousHistory = () => {
    commandQueue.replace(commandQueue.slice(currentPosition.get()));
    currentPosition.set(0);
};

export const updateHistoryMessage = message =>
    commandQueue[currentPosition.get()].message = message;
