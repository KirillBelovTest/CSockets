(async function(){
    const button = document.createElement('button');
    const urlField = document.createElement('input'); 
    urlField.type = 'text'; 
    urlField.value = 'http://localhost:8000'; 
    button.innerText = 'Press'; 
    button.style.width = '300px'; 

    const body = document.body; 

    body.appendChild(urlField); 
    body.appendChild(button); 

    button.addEventListener('click', async () => {
        const time = performance.now(); 
        const response = await fetch(urlField.value); 
        const result = await response.blob(); 
        button.innerText = `Time: ${perfomance.now() - time} ms, Bytes: ${result.size}`; 
        console.log(result); 
    })
})()
