import os
import smtplib
import ssl
from email.message import EmailMessage

import dotenv

dotenv.load_dotenv()


class EmailHandler:
    port = 465  # For SSL
    smtp_server = "smtp.gmail.com"
    sender_email = os.environ["EMAIL_SENDER_ADDRESS"]
    domain = os.environ["SERVER_DOMAIN"]
    password = os.environ["EMAIL_APP_PASSWORD"]

    context = ssl.create_default_context()

    @classmethod
    def send_verification_email(
        EmailHandler: "EmailHandler", receiver_email: str, activation_id: str
    ) -> None:
        message = f"From: Talent Tree Manager <{EmailHandler.sender_email}>\nTo: {receiver_email}\nSubject: Activate your Talent Tree Manager account\n\nClick the following link to activate your account: {EmailHandler.domain}/activate_account/{activation_id}"

        with smtplib.SMTP_SSL(
            EmailHandler.smtp_server,
            EmailHandler.port,
            context=EmailHandler.context,
        ) as server:
            server.login(EmailHandler.sender_email, EmailHandler.password)
            server.sendmail(
                EmailHandler.sender_email,
                receiver_email,
                message,
            )

        # msg = EmailMessage()
        # msg.set_content(message)

        # msg['Subject'] = 'Subject'
        # msg['From'] = "me@gmail.com"
        # msg['To'] = "you@gmail.com"

        # # Send the message via our own SMTP server.
        # server = smtplib.SMTP_SSL('smtp.gmail.com', 465)
        # server.login("me@gmail.com", "password")
        # server.send_message(msg)
        # server.quit()
