
export const newField = (name, type) => {

    // type is JSON Data or Plain Text (for now)

    browser.element('.glyphicon-plus').click();

    browser.waitForExist('input');
    browser.keys([name, 'Enter']);

    browser.waitForExist('button*=' + type);
    browser.element('button*=' + type).click();

};


export const save = () =>
    browser.element('.glyphicon-floppy-save').click();

export const remove = name => {
    browser.element('button*=' + name).click();
    browser.element('.glyphicon-remove').click();
};

export const hasKey = name =>
    browser.waitForExist('button*=' + name);

export const hasNoKey = name =>
    browser.waitForExist('button*=' + name, 500, true);


export const selectKey = name =>
    browser.element('button*=' + name).click();


export const setJSON = json => {

    browser.moveToObject('span*=(0 entries)');
    browser.element('.glyphicon-pencil').click();
    browser.waitForExist('input');

    browser.setValue('input', json);
    browser.submitForm('input');

};


export const refresh = name => {

    browser.waitForExist('.glyphicon-refresh');
    browser.element('.glyphicon-refresh').click();

};
