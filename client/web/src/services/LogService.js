import {LocalDateTime, DateTimeFormatter} from 'js-joda'
import {addCommandProcessor} from "./CommunicationService";

const logEntries = observable([]);

addCommandProcessor('log', action('log', entries => entries.forEach(l => logEntries.push(l))));

export const getLogEntries = () => logEntries;

