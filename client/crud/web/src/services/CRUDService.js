import {selectedKey, refreshKeys} from '../components/KeyList';
import {read, update, remove as removeKey} from 'bluzelle';
import {observe} from 'mobx';

export const activeValue = observable(undefined);


// Worry about undoing later
// But this is where we'd do it.

observe(selectedKey, ({newValue, oldValue}) => {

	activeValue.set(undefined);


	if(newValue !== undefined) {

		// We can say that if the value is an object, 
		// wrap in an OMR. See: JSONEditor.js.

		read(newValue).then(value =>
			activeValue.set(value));

	}

});


export const save = () => 
    update(selectedKey.get(), activeValue.get());


export const remove = () => new Promise(resolve => {

    const sk = selectedKey.get(); 
    selectedKey.set();

    return removeKey(sk).then(() => {
        reload().then(resolve);
    });

});


export const rename = (oldKey, newKey) => new Promise(resolve => {

    read(oldKey).then(v => {

        removeKey(oldKey).then(() => {

            update(newKey, v).then(() => {

            	const s = selectedKey;

                if(selectedKey.get() === oldKey) {

                    selectedKey.set(newKey);

                }

                reload().then(resolve);

            });

        });

    }); 

});
    

export const reload = () => new Promise(resolve => {

    refreshKeys().then(keys => {

        const sk = selectedKey.get(); 
        selectedKey.set();

        if(keys.includes(sk)) {

            selectedKey.set(sk);

        }

        resolve();

    });

});