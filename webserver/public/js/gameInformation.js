const gameInformationMessages = [
    "This is a game about collecting trash in the ocean",
    "You are one of many divers. You will all compete with each other to see who can collect the most trash",
    "Collecting one piece of trash is worth one point",
    "Your character will constantly swim forward. Use the controls on your mobile device to make your character turn",
    "Good luck and have fun!"
];
var currentMessage = 0;
var seenAll = 0;
var messageInterval;

function handleInformationMessages() {
    messageInterval = setInterval(displayNextMessage, 1000);
}

function displayNextMessage() {
    var messageElement = document.querySelector('#gameInformation');
    if (gameStarted && seenAll === 1) {
        clearInterval(messageInterval);
        document.querySelector('#orientationPrompt').classList.remove('hidden');
        document.querySelector('#promptBackground').classList.remove('hidden');
        setCurrentScreen('gameRunningScreen');
    }else if (currentMessage < gameInformationMessages.length) {
        messageElement.style.opacity = 0;
        setTimeout(() => {
            messageElement.innerHTML = gameInformationMessages[currentMessage++];
            messageElement.style.opacity=1;
        }, 500);
    } else {
        currentMessage = 0;
        seenAll = 1;
    }
}