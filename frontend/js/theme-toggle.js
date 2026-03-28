document.addEventListener('DOMContentLoaded', () => {
    const themeSwitch = document.getElementById('themeSwitch');

    if (themeSwitch) {
        // 1. Check for a saved theme in localStorage
        const currentTheme = localStorage.getItem('theme');
        if (currentTheme) {
            document.body.classList.add(currentTheme);
            
            // If the saved theme is dark, check the toggle
            if (currentTheme === 'dark-mode') {
                themeSwitch.checked = true;
            }
        }

        // 2. Listen for a click on the toggle
        themeSwitch.addEventListener('change', function() {
            if (this.checked) {
                // If checked, add dark-mode class and save preference
                document.body.classList.add('dark-mode');
                localStorage.setItem('theme', 'dark-mode');
            } else {
                // If unchecked, remove dark-mode class and save preference
                document.body.classList.remove('dark-mode');
                localStorage.setItem('theme', 'light-mode');
            }
        });
    }
});