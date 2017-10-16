import {setMaxNodes, settings} from 'services/SettingsService'
import {commandProcessors, clearSentCommands, getSentCommands, receiveCommand} from 'services/CommunicationService'



// describe('services/SettingsService', () => {
//     describe('setMaxNodes command processor', () => {
//         it('should be added on startup', () => {
//             expect(commandProcessors.setMaxNodes).not.to.be.undefined
//         });
//
//         it('should reactively update the local settings object', () => {
//             let maxNodes;
//
//            const stop = autorun(() => maxNodes = settings.maxNodes);
//             setMaxNodes(0);
//             expect(maxNodes).to.equal(0);
//
//             receiveCommand('setMaxNodes', 99);
//             expect(maxNodes).to.equal(99);
//             stop();
//         });
//     });
//
//
//     describe('setMaxNodes()', () => {
//         beforeEach(clearSentCommands);
//         it('should send a setMaxNodes() command', () => {
//             expect(getSentCommands().length).to.equal(0);
//             setMaxNodes(50);
//             expect(getSentCommands()[0]).to.deep.equal({cmd: 'setMaxNodes', data: 50});
//         })
//
//     })
// });