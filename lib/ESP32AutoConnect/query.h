String query0 = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Enter Credentials</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    html, body {
      height: 100%;
      font-family: Roboto, sans-serif;
      font-size: 12pt;
      overflow: hidden;
      background-color: #171a1c;
    }

    body {
      display: grid;
      grid-template-columns: 1fr;
      grid-template-rows: 1fr;
      align-items: center;
      justify-items: center;
    }

    #panel {
      width: 18rem;
      justify-items: center;
      padding: 1rem 0 1rem 0;
      background-color: #29363d;
      border-radius: 1rem;
      box-shadow: 0 8px 16px rgba(0, 0, 0, 0.4), inset 0 2px 4px rgba(159, 202, 223, 0.25);
    }  

    h1 {
      font-size: 1.5rem;
      text-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
      text-align: center;
    }
    button {
      background-color: #4CAF50;
      width: 100%;
      color: white;
      padding: 15px;
      margin: 10px 0px;
      border: none;
      cursor: pointer;
      border-radius: 7px;
    }
    form {
      display: grid;
      color: #4CAF50;
      margin: 0rem 1rem 0rem 1rem
    }
    input[type=text], input[type=password] {
      width: 100%;
      margin: 8px 0;
      padding: 12px 20px;
      display: inline-block;
      border: 2px solid green;
      box-sizing: border-box;
      border-radius: 7px;
    }
    button:hover {
      opacity: 0.8;
    }

    .display {
      display: grid;
      grid-gap: .25rem;
    }

    .inset {
      color: #668899;
      text-shadow: -1px -1px 0 #000;
    }
    select {
      width: 100%;
      margin: 8px 0;
      padding: 12px 20px;
      display: inline-block;
      border: 2px solid green;
      box-sizing: border-box;
      border-radius: 7px;
    }    
  </style>
</head>
<body>
  <div id="panel">
    <h1 class="inset">AutoConnect</h1>
    <div class="display">
      <form action="/get">
          <label>SSID : </label>
          <select id="ssid" name="ssid">
            {n}
          </select>
          <label>Password : </label>
          <input type="text" placeholder="Enter Password" name="password" required>
          <button type="submit">Connect</button> 
      </form>
  </div>
  <script>

  </script>
</body>
</html>)rawliteral";