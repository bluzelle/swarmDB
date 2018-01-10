import PropTypes from 'prop-types';

// Bypasses all command queue stuff and just executes the command.
export const SimpleExecute = {
    context: {execute: ({doIt}) => doIt()},
    childContextTypes: {execute: PropTypes.func}
};


// Verifies that undoing and redoing has expected behavior.

// verifyBefore and verifyAfter should assert that the component
// is in the unaltered and altered state respectfully.
export const UndoRedoTest = ({verifyBefore, verifyAfter}) => ({
    context: {
        execute: ({doIt, undoIt}) => {

            verifyBefore();
            doIt();
            verifyAfter();
            undoIt();
            verifyBefore();
            doIt();
            verifyAfter();

        }
    },
    childContextTypes: {execute: PropTypes.func}
});